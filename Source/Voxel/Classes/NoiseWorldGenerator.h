// Copyright 2017 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelMaterial.h"
#include "VoxelWorldGenerator.h"
#include "FastNoise.h"
#include "NoiseWorldGenerator.generated.h"

/**
 *
 */
UCLASS(Blueprintable)
class VOXEL_API UNoiseWorldGenerator : public UVoxelWorldGenerator
{
	GENERATED_BODY()

public:
	UNoiseWorldGenerator();

	virtual void GetValuesAndMaterials(float Values[], FVoxelMaterial Materials[], const FIntVector& Start, const FIntVector& StartIndex, const int Step, const FIntVector& Size, const FIntVector& ArraySize) const override;
	virtual void SetVoxelWorld(AVoxelWorld* VoxelWorld) override;

private:
	FastNoise Noise;
};
