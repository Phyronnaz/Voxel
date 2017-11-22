// Copyright 2017 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelMaterial.h"
#include "VoxelWorldGenerator.h"
#include "SphereWorldGenerator.generated.h"

/**
 *
 */
UCLASS(Blueprintable)
class VOXEL_API USphereWorldGenerator : public UVoxelWorldGenerator
{
	GENERATED_BODY()

public:
	USphereWorldGenerator();

	virtual void GetValuesAndMaterials(float Values[], FVoxelMaterial Materials[], const FIntVector& Start, const FIntVector& StartIndex, const int Step, const FIntVector& Size, const FIntVector& ArraySize) const override;

	virtual void SetVoxelWorld(AVoxelWorld* VoxelWorld) override;
	virtual FVector GetUpVector(int X, int Y, int Z) const override;

	// Radius of the sphere in world space
	UPROPERTY(EditAnywhere)
		float Radius;

	UPROPERTY(EditAnywhere)
		FVoxelMaterial DefaultMaterial;

	// If true, sphere is a hole in a full world
	UPROPERTY(EditAnywhere)
		bool InverseOutsideInside;

	UPROPERTY(EditAnywhere)
		float HardnessMultiplier;

private:
	float LocalRadius;
};
