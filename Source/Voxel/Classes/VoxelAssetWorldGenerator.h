// Copyright 2017 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelMaterial.h"
#include "VoxelBox.h"
#include "VoxelWorldGenerator.h"
#include "VoxelAsset.h"
#include "Templates/SubclassOf.h"
#include "VoxelAssetWorldGenerator.generated.h"

// TODO: UBoundedVoxelWorldGenerator

/**
*
*/
UCLASS(Blueprintable)
class VOXEL_API UVoxelAssetWorldGenerator : public UVoxelWorldGenerator
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
		UVoxelAsset* Asset;

	UPROPERTY(EditAnywhere)
		TSubclassOf<UVoxelWorldGenerator> DefaultWorldGenerator;


	UVoxelAssetWorldGenerator();
	~UVoxelAssetWorldGenerator();

	virtual void GetValuesAndMaterials(float Values[], FVoxelMaterial Materials[], const FIntVector& Start, const FIntVector& StartIndex, const int Step, const FIntVector& Size, const FIntVector& ArraySize) const override;
	virtual void SetVoxelWorld(AVoxelWorld* VoxelWorld) override;

private:
	UPROPERTY()
		UVoxelWorldGenerator* InstancedWorldGenerator;

	FDecompressedVoxelAsset* DecompressedAsset;
	FVoxelBox Bounds;

	void CreateGeneratorAndDecompressedAsset(const float VoxelSize);
};
