// Copyright 2017 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelWorld.h"
#include "VoxelDataAsset.h"
#include "Components/StaticMeshComponent.h"
#include "VoxelWorldSectionImporter.generated.h"

/**
 *
 */
UCLASS(BlueprintType, HideCategories = ("Tick", "Replication", "Input", "Actor", "Rendering"))
class VOXEL_API AVoxelWorldSectionImporter : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, meta = (DisplayName = "Save Path: /Game/", RelativeToGameContentDir), Category = "Save configuration")
		FDirectoryPath SavePath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save configuration")
		FString FileName;



	UPROPERTY(EditAnywhere, Category = "Import configuration")
		AVoxelWorld* World;

	UPROPERTY(EditAnywhere, Category = "Import configuration")
		FIntVector TopCorner;

	UPROPERTY(EditAnywhere, Category = "Import configuration")
		FIntVector BottomCorner;

	// Optional
	UPROPERTY(EditAnywhere, Category = "Import configuration")
		AActor* TopActor;

	// Optional
	UPROPERTY(EditAnywhere, Category = "Import configuration")
		AActor* BottomActor;

	AVoxelWorldSectionImporter();

	void ImportToAsset(FDecompressedVoxelDataAsset& Asset);
	void SetCornersFromActors();

protected:
#if WITH_EDITOR
	void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	void Tick(float Deltatime) override;
#endif
};