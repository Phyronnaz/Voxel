// Copyright 2017 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelBox.h"
#include "TransitionDirection.h"
#include <deque>
#include "VoxelProceduralMeshComponent.h"

class AVoxelWorld;
class FVoxelData;
class FChunkOctree;
class UVoxelChunkComponent;
class UVoxelProceduralMeshComponent;
class FCollisionMeshHandler;
class FAsyncPolygonizerTask;

struct FChunkToDelete
{
	UVoxelChunkComponent* Chunk;
	float TimeLeft;

	FChunkToDelete(UVoxelChunkComponent* Chunk, float TimeLeft)
		: Chunk(Chunk)
		, TimeLeft(TimeLeft)
	{
	};
};

class FAsyncCollisionTask : public FNonAbandonableTask
{
public:
	// Output
	FVoxelProcMeshSection Section;

	const bool bEnableRender;
	const FIntVector ChunkPosition;
	FVoxelData* const Data;

	FAsyncCollisionTask(FVoxelData* Data, FIntVector ChunkPosition, bool bEnableRender);

	void DoWork();

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FAsyncFoliageTask, STATGROUP_ThreadPoolAsyncTasks);
	};
};

/**
 *
 */
class FVoxelRender
{
public:
	AVoxelWorld* const World;
	AActor* const ChunksParent;
	FVoxelData* const Data;

	FQueuedThreadPool* const MeshThreadPool;
	FQueuedThreadPool* const FoliageThreadPool;


	FVoxelRender(AVoxelWorld* World, AActor* ChunksParent, FVoxelData* Data, uint32 MeshThreadCount, uint32 FoliageThreadCount);
	~FVoxelRender();


	void Tick(float DeltaTime);

	void AddInvoker(TWeakObjectPtr<UVoxelInvokerComponent> Invoker);

	UVoxelChunkComponent* GetInactiveChunk();

	void UpdateChunk(FChunkOctree* Chunk, bool bAsync);
	void UpdateChunksAtPosition(FIntVector Position, bool bAsync);
	void UpdateChunksOverlappingBox(FVoxelBox Box, bool bAsync);
	void ApplyUpdates();

	void UpdateAll(bool bAsync);

	void UpdateLOD();


	void AddFoliageUpdate(UVoxelChunkComponent* Chunk);
	void AddApplyNewMesh(UVoxelChunkComponent* Chunk);
	void AddApplyNewFoliage(UVoxelChunkComponent* Chunk);

	// Not the same as the queues above, as it is emptied at the same frame: see ApplyUpdates
	void AddTransitionCheck(UVoxelChunkComponent* Chunk);


	void ScheduleDeletion(UVoxelChunkComponent* Chunk);
	void ChunkHasBeenDestroyed(UVoxelChunkComponent* Chunk);

	FChunkOctree* GetChunkOctreeAt(FIntVector Position) const;

	FChunkOctree* GetAdjacentChunk(TransitionDirection Direction, FIntVector Position, int Size) const;

	int GetDepthAt(FIntVector Position) const;

	// MUST be called before delete
	void Destroy();

	// Needed when ChunksParent != World
	FVector GetGlobalPosition(FIntVector LocalPosition);

private:

	// Chunks waiting for update
	TSet<FChunkOctree*> ChunksToUpdate;
	// Ids of the chunks that need to be updated synchronously
	TSet<uint64> IdsOfChunksToUpdateSynchronously;

	// Shared ptr because each ChunkOctree need a reference to itself, and the Main one isn't the child of anyone
	TSharedPtr<FChunkOctree> MainOctree;

	std::deque<UVoxelChunkComponent*> InactiveChunks;
	TSet<UVoxelChunkComponent*> ActiveChunks;

	TSet<UVoxelChunkComponent*> FoliageUpdateNeeded;

	TSet<UVoxelChunkComponent*> ChunksToCheckForTransitionChange;

	TSet<UVoxelChunkComponent*> ChunksToApplyNewMesh;
	FCriticalSection ChunksToApplyNewMeshLock;

	TSet<UVoxelChunkComponent*> ChunksToApplyNewFoliage;
	FCriticalSection ChunksToApplyNewFoliageLock;

	std::deque<FChunkToDelete> ChunksToDelete;

	// Invokers
	std::deque<TWeakObjectPtr<UVoxelInvokerComponent>> VoxelInvokerComponents;


	TArray<FCollisionMeshHandler*> CollisionComponents;


	float TimeSinceFoliageUpdate;
	float TimeSinceLODUpdate;
	float TimeSinceCollisionUpdate;
	bool bNeedToEndCollisionsTasks;

	void RemoveFromQueues(UVoxelChunkComponent* Chunk);
};
