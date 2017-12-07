// Copyright 2017 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "Octree.h"
#include "VoxelSave.h"
#include <deque>

class UVoxelWorldGenerator;
struct FVoxelAsset;

/**
 * Octree that holds modified values & colors
 */
class FValueOctree : public FOctree
{
public:
	/**
	 * Constructor
	 * @param	Position		Position (center) of this in voxel space
	 * @param	Depth			Distance to the highest resolution
	 * @param	WorldGenerator	Generator of the current world
	 */
	FValueOctree(UVoxelWorldGenerator* WorldGenerator, FIntVector Position, uint8 Depth, uint64 Id);
	~FValueOctree();

	// Generator for this world
	UVoxelWorldGenerator* WorldGenerator;

	/**
	 * Does this chunk have been modified?
	 * @return	Whether or not this chunk is dirty
	 */
	FORCEINLINE bool IsDirty() const;

	/**
	 * Get value and color at position
	 * @param	GlobalPosition	Position in voxel space
	 * @return	Value
	 * @return	Color
	 */
	void GetValuesAndMaterials(float Values[], FVoxelMaterial Materials[], const FIntVector& Start, const FIntVector& StartIndex, const int Step, const FIntVector& Size, const FIntVector& ArraySize) const;

	void SetValueAndMaterial(int X, int Y, int Z, float Value, FVoxelMaterial Material, bool bSetValue, bool bSetMaterial);

	/**
	 * Add dirty chunks to SaveList
	 * @param	SaveList		List to save chunks into
	 */
	void AddDirtyChunksToSaveList(std::deque<TSharedRef<FVoxelChunkSave>>& SaveList);
	/**
	 * Load chunks from SaveArray
	 * @param	SaveArray	Array to load chunks from
	 */
	void LoadFromSaveAndGetModifiedPositions(std::deque<FVoxelChunkSave>& Save, std::deque<FIntVector>& OutModifiedPositions);

	/**
	* Get direct child that owns GlobalPosition
	* @param	GlobalPosition	Position in voxel space
	*/
	FORCEINLINE FValueOctree* GetChild(int X, int Y, int Z) const;

	FORCEINLINE FValueOctree* GetLeaf(int X, int Y, int Z);

	/**
	 * Queue update of dirty chunks
	 * @param	World	Voxel world
	 */
	void GetDirtyChunksPositions(std::deque<FIntVector>& OutPositions);

private:
	/*
	Childs of this octree in the following order:

	bottom      top
	-----> y
	| 0 | 2    4 | 6
	v 1 | 3    5 | 7
	x
	*/
	TArray<FValueOctree*, TFixedAllocator<8>> Childs;

	// Values if dirty
	float Values[16 * 16 * 16];
	// Materials if dirty
	FVoxelMaterial Materials[16 * 16 * 16];

	bool bIsDirty;

	/**
	 * Create childs of this octree
	 */
	void CreateChilds();

	/**
	 * Init arrays
	 */
	void SetAsDirty();

	FORCEINLINE int IndexFromCoordinates(int X, int Y, int Z) const;

	FORCEINLINE void CoordinatesFromIndex(int Index, int& OutX, int& OutY, int& OutZ) const;
};