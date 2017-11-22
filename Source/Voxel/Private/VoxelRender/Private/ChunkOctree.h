// Copyright 2017 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "Octree.h"
#include "VoxelBox.h"

class UVoxelChunkComponent;
class FVoxelRender;
class UVoxelInvokerComponent;

/**
 * Create the octree for rendering and spawn VoxelChunks
 */
class FChunkOctree : public FOctree
{
public:
	FChunkOctree(FVoxelRender* Render, FIntVector Position, uint8 Depth, uint64 Id);


	FVoxelRender* const Render;

	/**
	 * Unload VoxelChunk if created and recursively delete childs
	 */
	void Delete();

	/**
	 * Create/Update the octree for the new position
	 * @param	World			Current VoxelWorld
	 * @param	Invokers		List of voxel invokers
	 */
	void UpdateLOD(const std::deque<TWeakObjectPtr<UVoxelInvokerComponent>>& Invokers);

	/**
	 * Get the leaf chunk at PointPosition.
	 * @param	PointPosition	Position in voxel space. Must be contained in this octree
	 * @return	Leaf chunk at PointPosition
	 */
	FChunkOctree* GetLeaf(const FIntVector& PointPosition);

	/**
	 * Get the VoxelChunk of this
	 * @return	VoxelChunk; can be nullptr
	 */
	UVoxelChunkComponent* GetVoxelChunk() const;

	/**
	* Get direct child at position. Must not be leaf
	* @param	PointPosition	Position in voxel space. Must be contained in this octree
	* @return	Direct child in which PointPosition is contained
	*/
	FChunkOctree* GetChild(const FIntVector& PointPosition);

	void GetLeafsOverlappingBox(const FVoxelBox& Box, std::deque<FChunkOctree*>& Octrees);

private:
	/*
	Childs of this octree in the following order:

	bottom      top
	-----> y
	| 0 | 2    4 | 6
	v 1 | 3    5 | 7
	x
	*/
	TArray<FChunkOctree*, TFixedAllocator<8>> Childs;

	// Is VoxelChunk created?
	bool bHasChunk;

	// Pointer to the chunk
	UVoxelChunkComponent* VoxelChunk;

	/**
	 * Create the VoxelChunk
	 */
	void Load();
	/**
	 * Unload the VoxelChunk
	 */
	void Unload();

	/**
	 * Create childs of this octree
	 */
	void CreateChilds();
	/**
	 * Delete childs (with their chunks)
	 */
	void DeleteChilds();
};