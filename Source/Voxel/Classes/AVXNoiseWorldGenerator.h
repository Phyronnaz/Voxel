// Copyright 2017 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelMaterial.h"
#include "VoxelWorldGenerator.h"
#include "FastNoise.h"
#include "FastNoiseSIMD.h"
#include "AVXNoiseWorldGenerator.generated.h"

/**
 *
 */
UCLASS(Blueprintable)
class VOXEL_API UAVXNoiseWorldGenerator : public UVoxelWorldGenerator
{
	GENERATED_BODY()

public:
	UAVXNoiseWorldGenerator();

	virtual void GetValuesAndMaterials(float Values[], FVoxelMaterial Materials[], const FIntVector& Start, const FIntVector& StartIndex, const int Step, const FIntVector& Size, const FIntVector& ArraySize) const override;
	virtual void SetVoxelWorld(AVoxelWorld* VoxelWorld) override;

private:
	FastNoise Noise;
	FastNoiseSIMD* NoiseSIMD;
};
