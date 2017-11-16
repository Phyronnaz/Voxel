// Copyright 2017 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelMaterial.h"
#include "VoxelWorldGenerator.generated.h"

class AVoxelWorld;

// TODO: GetValueAndMaterial

/**
 *
 */
UCLASS(Blueprintable, abstract, HideCategories = ("Tick", "Replication", "Input", "Actor", "Rendering"))
class VOXEL_API UVoxelWorldGenerator : public UObject
{
	GENERATED_BODY()

public:

	/**
	 * Values or Materials can be null
	 */
	virtual void GetValuesAndMaterials(float Values[], FVoxelMaterial Materials[], const FIntVector& Start, const FIntVector& StartIndex, const int Step, const FIntVector& Size, const FIntVector& ArraySize) const
	{

	}

	/**
	 * If you need a reference to Voxel World
	 */
	virtual void SetVoxelWorld(AVoxelWorld* VoxelWorld)
	{

	}

	/**
	 * World up vector at position
	 */
	virtual FVector GetUpVector(int X, int Y, int Z) const
	{
		return FVector::UpVector;
	}
};
