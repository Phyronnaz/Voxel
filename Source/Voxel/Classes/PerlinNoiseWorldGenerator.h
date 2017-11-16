// Copyright 2017 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelWorldGenerator.h"
#include "FastNoise/FastNoise.h"
#include "FastNoise/FastNoiseSIMD/FastNoiseSIMD.h"
#include "PerlinNoiseWorldGenerator.generated.h"

/**
 *
 */
UCLASS(Blueprintable)
class VOXEL_API UPerlinNoiseWorldGenerator : public UVoxelWorldGenerator
{
	GENERATED_BODY()

public:
	UPerlinNoiseWorldGenerator();

	virtual void GetValuesAndMaterials(float Values[], FVoxelMaterial Materials[], const FIntVector& Start, const FIntVector& StartIndex, const int Step, const FIntVector& Size, const FIntVector& ArraySize) const override;
	virtual void SetVoxelWorld(AVoxelWorld* VoxelWorld) override;

private:
	FastNoise Noise;
	FastNoiseSIMD* NoiseSIMD;
};
