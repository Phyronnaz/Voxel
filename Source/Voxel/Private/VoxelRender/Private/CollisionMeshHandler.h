// Copyright 2017 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelProceduralMeshComponent.h"

class FVoxelData;
class FQueuedThreadPool;

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

class FCollisionMeshHandler
{
public:
	TWeakObjectPtr<UVoxelInvokerComponent> const Invoker;
	AVoxelWorld* const World;
	FQueuedThreadPool* const Pool;

	FCollisionMeshHandler(TWeakObjectPtr<UVoxelInvokerComponent> const Invoker, AVoxelWorld* const World, FQueuedThreadPool* const Pool);
	~FCollisionMeshHandler();

	void StartTasksTick();
	void EndTasksTick();

	FORCEINLINE bool IsValid();

	void Destroy();

	void UpdateInBox(FVoxelBox Box);

private:
	FIntVector CurrentCenter;
	UVoxelProceduralMeshComponent* Components[2][2][2];
	FAsyncTask<FAsyncCollisionTask>* Tasks[2][2][2];
	TSet<FIntVector> ChunksToUpdate;

	void Update(bool bXMax, bool bYMax, bool bZMax);
};