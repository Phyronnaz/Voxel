// Copyright 2017 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelMaterial.h"
#include "VoxelBox.h"
#include "VoxelSave.h"
#include "VoxelGrassType.h"
#include "VoxelWorldGenerator.h"
#include "GameFramework/Actor.h"
#include "Templates/SubclassOf.h"
#include "VoxelWorld.generated.h"

using namespace UP;
using namespace UM;
using namespace US;
using namespace UC;

class FVoxelRender;
class FVoxelData;
class UVoxelInvokerComponent;
class AVoxelWorldEditorInterface;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnClientConnection);

/**
 * Voxel World actor class
 */
UCLASS()
class VOXEL_API AVoxelWorld : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Voxel")
		TArray<UVoxelGrassType*> GrassTypes;

	UPROPERTY(BlueprintAssignable)
		FOnClientConnection OnClientConnection;

	// Dirty hack to get a ref to AVoxelWorldEditor::StaticClass()
	UClass* VoxelWorldEditorClass;

	AVoxelWorld();
	virtual ~AVoxelWorld() override;

	void CreateInEditor();
	void DestroyInEditor();

	void AddInvoker(TWeakObjectPtr<UVoxelInvokerComponent> Invoker);

	FORCEINLINE AVoxelWorldEditorInterface	* GetVoxelWorldEditor() const;
	FORCEINLINE FVoxelData* GetData() const;
	FORCEINLINE UVoxelWorldGenerator* GetWorldGenerator() const;
	FORCEINLINE int32 GetSeed() const;
	FORCEINLINE float GetFoliageFPS() const;
	FORCEINLINE float GetLODUpdateFPS() const;
	FORCEINLINE float GetCollisionUpdateFPS() const;
	FORCEINLINE UMaterialInterface* GetVoxelMaterial() const;
	FORCEINLINE bool GetComputeTransitions() const;
	FORCEINLINE bool GetComputeCollisions() const;
	FORCEINLINE bool GetComputeExtendedCollisions();
	FORCEINLINE uint8 GetMaxDepthToGenerateCollisions() const;
	FORCEINLINE bool GetDebugCollisions() const;
	FORCEINLINE float GetDeletionDelay() const;
	FORCEINLINE float GetNormalThresholdForSimplification() const;
	FORCEINLINE bool GetEnableAmbientOcclusion() const;
	FORCEINLINE int GetRayMaxDistance() const;
	FORCEINLINE int GetRayCount() const;


	UFUNCTION(BlueprintCallable, Category = "Voxel")
		bool IsCreated() const;

	UFUNCTION(BlueprintCallable, Category = "Voxel")
		int GetDepthAt(const FIntVector& Position) const;

	// Size of a voxel in cm
	UFUNCTION(BlueprintCallable, Category = "Voxel")
		float GetVoxelSize() const;

	// Size of this world
	UFUNCTION(BlueprintCallable, Category = "Voxel")
		int Size() const;

	/**
	 * Convert position from world space to voxel space
	 * @param	Position	Position in world space
	 * @return	Position in voxel space
	 */
	UFUNCTION(BlueprintCallable, Category = "Voxel")
		FIntVector GlobalToLocal(const FVector& Position) const;

	UFUNCTION(BlueprintCallable, Category = "Voxel")
		FVector LocalToGlobal(const FIntVector& Position) const;

	FVector LocalToGlobal(const FVector& Position) const;

	UFUNCTION(BlueprintCallable, Category = "Voxel")
		TArray<FIntVector> GetNeighboringPositions(const FVector& GlobalPosition) const;

	/**
	 * Add chunk to update queue that will be processed at the end of the frame
	 * @param	Position	Position in voxel space
	 * @param	bAsync		Async update?
	 */
	UFUNCTION(BlueprintCallable, Category = "Voxel")
		void UpdateChunksAtPosition(const FIntVector& Position, bool bAsync);

	UFUNCTION(BlueprintCallable, Category = "Voxel")
		void UpdateChunksOverlappingBox(const FVoxelBox& Box, bool bAsync);

	UFUNCTION(BlueprintCallable, Category = "Voxel")
		void UpdateAll(bool bAsync);

	/**
	 * Is position in this world?
	 * @param	Position	Position in voxel space
	 * @return	IsInWorld?
	 */
	UFUNCTION(BlueprintCallable, Category = "Voxel")
		bool IsInWorld(const FIntVector& Position) const;

	UFUNCTION(BlueprintCallable, Category = "Voxel")
		bool GetIntersection(const FIntVector& Start, const FIntVector& End, FVector& OutGlobalPosition, FIntVector& OutVoxelPosition);

	UFUNCTION(BlueprintCallable, Category = "Voxel")
		FVector GetNormal(const FIntVector& Position) const;

	/**
	 * Get value at position
	 * @param	Position	Position in voxel space
	 * @return	Value at position
	 */
	UFUNCTION(BlueprintCallable, Category = "Voxel")
		float GetValue(const FIntVector& Position) const;
	/**
	 * Get material at position
	 * @param	Position	Position in voxel space
	 */
	UFUNCTION(BlueprintCallable, Category = "Voxel")
		FVoxelMaterial GetMaterial(const FIntVector& Position) const;

	/**
	 * Set value at position
	 * @param	Position	Position in voxel space
	 * @param	Value		Value to set
	 */
	UFUNCTION(BlueprintCallable, Category = "Voxel")
		void SetValue(const FIntVector& Position, float Value);
	/**
	 * Set material at position
	 * @param	Position	Position in voxel space
	 * @param	Material	FVoxelMaterial
	 */
	UFUNCTION(BlueprintCallable, Category = "Voxel")
		void SetMaterial(const FIntVector& Position, const FVoxelMaterial& Material);

	/**
	 * Get array to save world
	 * @return	SaveArray
	 */
	UFUNCTION(BlueprintCallable, Category = "Voxel")
		void GetSave(FVoxelWorldSave& OutSave) const;
	/**
	 * Load world from save
	 * @param	Save	Save to load from
	 * @param	bReset	Reset existing world? Set to false only if current world is unmodified
	 */
	UFUNCTION(BlueprintCallable, Category = "Voxel")
		void LoadFromSave(const FVoxelWorldSave& Save, bool bReset = true);

protected:
	// Called when the game starts or when spawned
	void BeginPlay() override;
	void Tick(float DeltaTime) override;
	void BeginDestroy() override;

#if WITH_EDITOR
	bool ShouldTickIfViewportsOnly() const override;
	void PostLoad() override;
#endif

private:
	UPROPERTY(EditAnywhere, Category = "Voxel")
		UMaterialInterface* VoxelMaterial;

	// Width = 16 * 2^Depth
	UPROPERTY(EditAnywhere, Category = "Voxel", meta = (ClampMin = "0", ClampMax = "19", UIMin = "0", UIMax = "19", DisplayName = "Depth"))
		int NewDepth;

	// Size of a voxel in cm
	UPROPERTY(EditAnywhere, Category = "Voxel", meta = (DisplayName = "Voxel Size"))
		float NewVoxelSize;

	// Generator for this world
	UPROPERTY(EditAnywhere, Category = "Voxel")
		TSubclassOf<UVoxelWorldGenerator> WorldGenerator;

	UPROPERTY(EditAnywhere, Category = "Voxel", meta = (ClampMin = "1", UIMin = "1"))
		int32 Seed;

	UPROPERTY(EditAnywhere, Category = "Voxel", AdvancedDisplay, meta = (ClampMin = "0", UIMin = "0"))
		float NormalThresholdForSimplification;

	UPROPERTY(EditAnywhere, Category = "Ambient Occlusion")
		bool bEnableAmbientOcclusion;

	UPROPERTY(EditAnywhere, Category = "Ambient Occlusion", meta = (EditCondition = "bEnableAmbientOcclusion"))
		int RayCount;

	UPROPERTY(EditAnywhere, Category = "Ambient Occlusion", meta = (EditCondition = "bEnableAmbientOcclusion"))
		int RayMaxDistance;

	// Time to wait before deleting old chunks to avoid holes
	UPROPERTY(EditAnywhere, Category = "Voxel", meta = (ClampMin = "0", UIMin = "0"), AdvancedDisplay)
		float DeletionDelay;

	UPROPERTY(EditAnywhere, Category = "Voxel", AdvancedDisplay)
		bool bComputeTransitions;

	UPROPERTY(EditAnywhere, Category = "Voxel", AdvancedDisplay)
		bool bComputeExtendedCollisions;

	// Inclusive
	UPROPERTY(EditAnywhere, Category = "Voxel", AdvancedDisplay, meta = (EditCondition = "bComputeExtendedCollisions"))
		uint8 MaxDepthToGenerateCollisions;

	UPROPERTY(EditAnywhere, Category = "Voxel", AdvancedDisplay)
		bool bDebugCollisions;

	UPROPERTY(EditAnywhere, Category = "Voxel", AdvancedDisplay)
		float FoliageFPS;

	UPROPERTY(EditAnywhere, Category = "Voxel", AdvancedDisplay)
		float LODUpdateFPS;

	UPROPERTY(EditAnywhere, Category = "Voxel", AdvancedDisplay)
		float CollisionUpdateFPS;

	UPROPERTY(EditAnywhere, Category = "Voxel", meta = (ClampMin = "1", UIMin = "1"), AdvancedDisplay)
		int MeshThreadCount;

	UPROPERTY(EditAnywhere, Category = "Voxel", meta = (ClampMin = "1", UIMin = "1"), AdvancedDisplay)
		int FoliageThreadCount;


	UPROPERTY()
		UVoxelWorldGenerator* InstancedWorldGenerator;

	UPROPERTY()
		AVoxelWorldEditorInterface* VoxelWorldEditor;

	FVoxelData* Data;
	FVoxelRender* Render;

	bool bIsCreated;

	int Depth;
	float VoxelSize;

	bool bComputeCollisions;

	float TimeSinceSync;

	void CreateWorld();
	void DestroyWorld();
};
