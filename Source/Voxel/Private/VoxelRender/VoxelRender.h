// Copyright 2017 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelBox.h"
#include "Direction.h"
#include "VoxelProceduralMeshComponent.h"
#include <deque>

class AVoxelWorld;
class FVoxelData;
class FChunkOctree;
class UVoxelChunkComponent;
class FCollisionMeshHandler;
class UVoxelInvokerComponent;

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
	FQueuedThreadPool* const CollisionThreadPool;


	FVoxelRender(AVoxelWorld* World, AActor* ChunksParent, FVoxelData* Data, uint32 MeshThreadCount, uint32 FoliageThreadCount);
	~FVoxelRender();


	void Tick(float DeltaTime);

	void AddInvoker(TWeakObjectPtr<UVoxelInvokerComponent> Invoker);

	UVoxelChunkComponent* GetInactiveChunk();

	void UpdateChunk(FChunkOctree* Chunk, bool bAsync);
	void UpdateChunksAtPosition(const FIntVector& Position, bool bAsync);
	void UpdateChunksOverlappingBox(const FVoxelBox& Box, bool bAsync);
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

	FChunkOctree* GetChunkOctreeAt(const FIntVector& Position) const;

	FChunkOctree* GetAdjacentChunk(EDirection Direction, const FIntVector& Position, int Size) const;

	int GetDepthAt(const FIntVector& Position) const;

	// MUST be called before delete
	void Destroy();

	// Needed when ChunksParent != World
	FVector GetGlobalPosition(const FIntVector& LocalPosition);

	void RemoveChunkFromQueue(FChunkOctree* Chunk);

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
