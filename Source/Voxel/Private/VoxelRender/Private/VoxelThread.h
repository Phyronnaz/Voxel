// Copyright 2017 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelGrassType.h"
#include "VoxelProceduralMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "IQueuedWork.h"

class FVoxelPolygonizer;
class UVoxelChunkComponent;
class FVoxelData;
class UVoxelWorldGenerator;

/**
 * Thread to create foliage
 */
class FAsyncFoliageTask : public FNonAbandonableTask
{
public:
	FVoxelGrassVariety GrassVariety;

	// Output
	FStaticMeshInstanceData InstanceBuffer;
	TArray<FClusterNode> ClusterTree;
	int OutOcclusionLayerNum;

	FAsyncFoliageTask(const FVoxelProcMeshSection& Section, const FVoxelGrassVariety& GrassVariety, int GrassVarietyIndex, uint8 Material, AVoxelWorld* World, const FIntVector& ChunkPosition, UVoxelChunkComponent* Chunk);

	void DoWork();

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FAsyncFoliageTask, STATGROUP_ThreadPoolAsyncTasks);
	};


private:
	UVoxelChunkComponent* Chunk;

	FVoxelProcMeshSection const Section;
	int GrassVarietyIndex;
	uint8 const Material;
	float const VoxelSize;
	FIntVector const ChunkPosition;
	int const Seed;
	UVoxelWorldGenerator* const Generator;
	AVoxelWorld* const World;
};


/**
 * Thread to create mesh
 */
class FAsyncPolygonizerTask : public IQueuedWork
{
public:
	UVoxelChunkComponent* const Chunk;
	FThreadSafeCounter DontDoCallback;

	// Will delete Builder when done
	FAsyncPolygonizerTask(UVoxelChunkComponent* Chunk);
	~FAsyncPolygonizerTask();

	void DoThreadedWork() override;
	void Abandon()  override;
};