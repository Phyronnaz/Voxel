// Copyright 2017 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelDataAsset.h"
#include "Components/StaticMeshComponent.h"
#include "VoxelMeshImporter.generated.h"

class UStaticMesh;

/**
 *
 */
UCLASS(BlueprintType, HideCategories = ("Tick", "Replication", "Input", "Actor", "Rendering"))
class VOXEL_API AVoxelMeshImporter : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, meta = (DisplayName = "Save Path: /Game/", RelativeToGameContentDir), Category = "Save configuration")
		FDirectoryPath SavePath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save configuration")
		FString FileName;



	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"), Category = "Import configuration")
		UStaticMesh* StaticMesh;

	// Size of the voxels used to voxelize the mesh
	UPROPERTY(EditAnywhere, Category = "Import configuration", meta = (ClampMin = "0", UIMin = "0"))
		float MeshVoxelSize;


	// Number of voxels of MeshVoxelSize size per real voxel = UpscalingFactor ** 3
	UPROPERTY(EditAnywhere, Category = "Import configuration", meta = (ClampMin = "2", UIMin = "2"))
		int UpscalingFactor;

	// One actor per part of the mesh
	UPROPERTY(EditAnywhere, Category = "Import configuration")
		TArray<AActor*> ActorsInsideTheMesh;

	UPROPERTY(EditAnywhere, Category = "Debug")
		bool bDrawPoints;

	AVoxelMeshImporter();

	void ImportToAsset(FDecompressedVoxelDataAsset& Asset);

protected:
#if WITH_EDITOR
	void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	UPROPERTY()
		UStaticMeshComponent* MeshComponent;
};