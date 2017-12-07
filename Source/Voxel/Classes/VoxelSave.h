// Copyright 2017 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelMaterial.h"
#include <deque>
#include "VoxelSave.generated.h"


struct FVoxelChunkSave
{
	uint64 Id;

	TArray<float, TFixedAllocator<16 * 16 * 16>> Values;

	TArray<FVoxelMaterial, TFixedAllocator<16 * 16 * 16>> Materials;

	FVoxelChunkSave();

	FVoxelChunkSave(uint64 Id, FIntVector Position, float Values[16 * 16 * 16], FVoxelMaterial Materials[16 * 16 * 16]);
};

FORCEINLINE FArchive& operator<<(FArchive &Ar, FVoxelChunkSave& Save)
{
	Ar << Save.Id;
	Ar << Save.Values;
	Ar << Save.Materials;

	return Ar;
}

USTRUCT(BlueprintType, Category = Voxel)
struct VOXEL_API FVoxelWorldSave
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere)
		int Depth;

	UPROPERTY()
		TArray<uint8> Data;


	FVoxelWorldSave();

	void Init(int NewDepth, const std::deque<TSharedRef<FVoxelChunkSave>>& ChunksList);

	std::deque<FVoxelChunkSave> GetChunksList() const;
};