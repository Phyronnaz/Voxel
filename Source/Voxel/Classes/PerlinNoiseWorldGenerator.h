// Copyright 2017 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelWorldGenerator.h"
#include "PerlinNoise.h"
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

	virtual float GetDefaultValue(int X, int Y, int Z) override;
	virtual FVoxelMaterial GetDefaultMaterial(int X, int Y, int Z) override;
	virtual void SetVoxelWorld(AVoxelWorld* VoxelWorld) override;

private:
	float GetNoise(float Frequency, int X, int Y, int Z);

	PerlinNoise Noise;
};
