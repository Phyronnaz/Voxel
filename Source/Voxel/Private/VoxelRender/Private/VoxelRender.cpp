// Copyright 2017 Phyronnaz

#pragma warning( disable : 4800 )

#include "VoxelRender.h"
#include "VoxelPrivate.h"
#include "VoxelWorld.h"
#include "VoxelData.h"
#include "ChunkOctree.h"
#include "VoxelChunkComponent.h"
#include <algorithm>
#include "CollisionMeshHandler.h"

DECLARE_CYCLE_STAT(TEXT("VoxelRender ~ ApplyUpdates"), STAT_ApplyUpdates, STATGROUP_Voxel);
DECLARE_CYCLE_STAT(TEXT("VoxelRender ~ UpdateLOD"), STAT_UpdateLOD, STATGROUP_Voxel);

FVoxelRender::FVoxelRender(AVoxelWorld* World, AActor* ChunksParent, FVoxelData* Data, uint32 MeshThreadCount, uint32 FoliageThreadCount)
	: World(World)
	, ChunksParent(ChunksParent)
	, Data(Data)
	, MeshThreadPool(FQueuedThreadPool::Allocate())
	, FoliageThreadPool(FQueuedThreadPool::Allocate())
	, CollisionThreadPool(FQueuedThreadPool::Allocate())
	, TimeSinceFoliageUpdate(0)
	, TimeSinceLODUpdate(0)
	, TimeSinceCollisionUpdate(0)
	, bNeedToEndCollisionsTasks(false)
{
	// Add existing chunks
	for (auto Component : ChunksParent->GetComponentsByClass(UVoxelChunkComponent::StaticClass()))
	{
		UVoxelChunkComponent* ChunkComponent = Cast<UVoxelChunkComponent>(Component);
		if (ChunkComponent)
		{
			ChunkComponent->SetProcMeshSection(0, FVoxelProcMeshSection());
			ChunkComponent->SetVoxelMaterial(World->GetVoxelMaterial());
			InactiveChunks.push_front(ChunkComponent);
		}
	}
	// Delete existing grass components
	for (auto Component : ChunksParent->GetComponentsByClass(UHierarchicalInstancedStaticMeshComponent::StaticClass()))
	{
		Component->DestroyComponent();
	}

	MeshThreadPool->Create(MeshThreadCount, 1024 * 1024);
	FoliageThreadPool->Create(FoliageThreadCount, 1024 * 1024);
	CollisionThreadPool->Create(1, 1024 * 1024);

	MainOctree = MakeShareable(new FChunkOctree(this, FIntVector::ZeroValue, Data->Depth, FOctree::GetTopIdFromDepth(Data->Depth)));
}

FVoxelRender::~FVoxelRender()
{
	check(ActiveChunks.Num() == 0);
	check(InactiveChunks.empty());

	for (auto Handler : CollisionComponents)
	{
		delete Handler;
	}
}

void FVoxelRender::Tick(float DeltaTime)
{
	check(ChunksToCheckForTransitionChange.Num() == 0);

	TimeSinceFoliageUpdate += DeltaTime;
	TimeSinceLODUpdate += DeltaTime;
	TimeSinceCollisionUpdate += DeltaTime;

	if (TimeSinceLODUpdate > 1 / World->GetLODUpdateFPS())
	{
		check(ChunksToCheckForTransitionChange.Num() == 0);

		UpdateLOD();

		// See Init and Unload functions of AVoxelChunk
		for (auto Chunk : ChunksToCheckForTransitionChange)
		{
			Chunk->CheckTransitions();
		}
		ChunksToCheckForTransitionChange.Empty();

		TimeSinceLODUpdate = 0;
	}

	if (TimeSinceCollisionUpdate > 0.5f / World->GetCollisionUpdateFPS())
	{
		TimeSinceCollisionUpdate = 0;
		for (auto& Handler : CollisionComponents)
		{
			if (Handler->IsValid())
			{
				if (!bNeedToEndCollisionsTasks)
				{
					Handler->StartTasksTick();
				}
				else
				{
					Handler->EndTasksTick();
				}
			}
			else
			{
				Handler->Destroy();
				delete Handler;
				Handler = nullptr;
			}
		}
		bNeedToEndCollisionsTasks = !bNeedToEndCollisionsTasks;
		CollisionComponents.RemoveAll([](void* P) { return P == nullptr; });
	}

	ApplyUpdates();

	if (TimeSinceFoliageUpdate > 1 / World->GetFoliageFPS())
	{
		for (auto Chunk : FoliageUpdateNeeded)
		{
			Chunk->UpdateFoliage();
		}
		FoliageUpdateNeeded.Empty();
		TimeSinceFoliageUpdate = 0;
	}

	// Apply new meshes and new foliages
	{
		FScopeLock Lock(&ChunksToApplyNewMeshLock);
		for (auto Chunk : ChunksToApplyNewMesh)
		{
			Chunk->ApplyNewMesh();
		}
		ChunksToApplyNewMesh.Empty();
	}
	{
		FScopeLock Lock(&ChunksToApplyNewFoliageLock);
		for (auto Chunk : ChunksToApplyNewFoliage)
		{
			Chunk->ApplyNewFoliage();
		}
		ChunksToApplyNewFoliage.Empty();
	}

	// Chunks to delete
	for (auto& ChunkToDelete : ChunksToDelete)
	{
		ChunkToDelete.TimeLeft -= DeltaTime;
		if (ChunkToDelete.TimeLeft < 0)
		{
			auto Chunk = ChunkToDelete.Chunk;
			check(!FoliageUpdateNeeded.Contains(Chunk));
			check(!ChunksToApplyNewMesh.Contains(Chunk));
			check(!ChunksToApplyNewFoliage.Contains(Chunk));
			Chunk->Delete();
			ActiveChunks.Remove(Chunk);
			InactiveChunks.push_front(Chunk);
		}
	}
	ChunksToDelete.erase(std::remove_if(ChunksToDelete.begin(), ChunksToDelete.end(), [](FChunkToDelete ChunkToDelete) { return ChunkToDelete.TimeLeft < 0; }), ChunksToDelete.end());
}

void FVoxelRender::AddInvoker(TWeakObjectPtr<UVoxelInvokerComponent> Invoker)
{
	if (Invoker.IsValid())
	{
		VoxelInvokerComponents.push_front(Invoker);
		if (World->GetComputeCollisions())
		{
			CollisionComponents.Add(new FCollisionMeshHandler(Invoker, World, CollisionThreadPool));
		}
	}
}

UVoxelChunkComponent* FVoxelRender::GetInactiveChunk()
{
	UVoxelChunkComponent* Chunk;
	if (InactiveChunks.empty())
	{
		Chunk = NewObject<UVoxelChunkComponent>(ChunksParent, NAME_None, RF_Transient | RF_NonPIEDuplicateTransient);
		Chunk->SetupAttachment(ChunksParent->GetRootComponent(), NAME_None);
		Chunk->RegisterComponent();
		Chunk->SetVoxelMaterial(World->GetVoxelMaterial());
	}
	else
	{
		Chunk = InactiveChunks.front();
		InactiveChunks.pop_front();
	}
	ActiveChunks.Add(Chunk);

	check(Chunk->IsValidLowLevel());
	return Chunk;
}

void FVoxelRender::UpdateChunk(FChunkOctree* Chunk, bool bAsync)
{
	ChunksToUpdate.Add(Chunk);
	if (!bAsync)
	{
		IdsOfChunksToUpdateSynchronously.Add(Chunk->Id);
	}
}

void FVoxelRender::UpdateChunksAtPosition(const FIntVector& Position, bool bAsync)
{
	check(Data->IsInWorld(Position.X, Position.Y, Position.Z));

	UpdateChunksOverlappingBox(FVoxelBox(Position, Position), bAsync);
}

void FVoxelRender::UpdateChunksOverlappingBox(const FVoxelBox& Box, bool bAsync)
{
	FVoxelBox ExtendedBox(Box.Min - FIntVector(2, 2, 2), Box.Max + FIntVector(2, 2, 2));

	std::deque<FChunkOctree*> OverlappingLeafs;
	MainOctree->GetLeafsOverlappingBox(ExtendedBox, OverlappingLeafs);

	for (auto Chunk : OverlappingLeafs)
	{
		UpdateChunk(Chunk, bAsync);
	}

	for (auto& Handler : CollisionComponents)
	{
		if (Handler->IsValid())
		{
			Handler->UpdateInBox(ExtendedBox);
		}
		else
		{
			Handler->Destroy();
			delete Handler;
			Handler = nullptr;
		}
	}

	CollisionComponents.RemoveAll([](void* P) { return P == nullptr; });
}

void FVoxelRender::ApplyUpdates()
{
	SCOPE_CYCLE_COUNTER(STAT_ApplyUpdates);

	for (auto& Chunk : ChunksToUpdate)
	{
		if (Chunk->GetVoxelChunk())
		{
			bool bAsync = !IdsOfChunksToUpdateSynchronously.Contains(Chunk->Id);
			bool bSuccess = Chunk->GetVoxelChunk()->Update(bAsync);

			/*if (!bSuccess)
			{
				UE_LOG(LogVoxel, Warning, TEXT("Chunk already updating"));
			}*/
		}
	}
	ChunksToUpdate.Reset();
	IdsOfChunksToUpdateSynchronously.Reset();
}

void FVoxelRender::UpdateAll(bool bAsync)
{
	for (auto Chunk : ActiveChunks)
	{
		Chunk->Update(bAsync);
	}
}

void FVoxelRender::UpdateLOD()
{
	SCOPE_CYCLE_COUNTER(STAT_UpdateLOD);

	// Clean
	VoxelInvokerComponents.erase(std::remove_if(VoxelInvokerComponents.begin(), VoxelInvokerComponents.end(), [](TWeakObjectPtr<UVoxelInvokerComponent> Ptr) { return !Ptr.IsValid(); }), VoxelInvokerComponents.end());

	MainOctree->UpdateLOD(VoxelInvokerComponents);
}

void FVoxelRender::AddFoliageUpdate(UVoxelChunkComponent* Chunk)
{
	FoliageUpdateNeeded.Add(Chunk);
}

void FVoxelRender::AddTransitionCheck(UVoxelChunkComponent* Chunk)
{
	ChunksToCheckForTransitionChange.Add(Chunk);
}

void FVoxelRender::ScheduleDeletion(UVoxelChunkComponent* Chunk)
{
	// Cancel any pending update
	RemoveFromQueues(Chunk);

	// Schedule deletion
	ChunksToDelete.push_front(FChunkToDelete(Chunk, World->GetDeletionDelay()));
}

void FVoxelRender::ChunkHasBeenDestroyed(UVoxelChunkComponent* Chunk)
{
	RemoveFromQueues(Chunk);

	ChunksToDelete.erase(std::remove_if(ChunksToDelete.begin(), ChunksToDelete.end(), [Chunk](FChunkToDelete ChunkToDelete) { return ChunkToDelete.Chunk == Chunk; }), ChunksToDelete.end());

	InactiveChunks.erase(std::remove(InactiveChunks.begin(), InactiveChunks.end(), Chunk), InactiveChunks.end());

	ActiveChunks.Remove(Chunk);
}

void FVoxelRender::AddApplyNewMesh(UVoxelChunkComponent* Chunk)
{
	FScopeLock Lock(&ChunksToApplyNewMeshLock);
	ChunksToApplyNewMesh.Add(Chunk);
}

void FVoxelRender::AddApplyNewFoliage(UVoxelChunkComponent* Chunk)
{
	FScopeLock Lock(&ChunksToApplyNewFoliageLock);
	ChunksToApplyNewFoliage.Add(Chunk);
}

void FVoxelRender::RemoveFromQueues(UVoxelChunkComponent* Chunk)
{
	FoliageUpdateNeeded.Remove(Chunk);

	{
		FScopeLock Lock(&ChunksToApplyNewMeshLock);
		ChunksToApplyNewMesh.Remove(Chunk);
	}
	{
		FScopeLock Lock(&ChunksToApplyNewFoliageLock);
		ChunksToApplyNewFoliage.Remove(Chunk);
	}
}

FChunkOctree* FVoxelRender::GetChunkOctreeAt(const FIntVector& Position) const
{
	check(Data->IsInWorld(Position.X, Position.Y, Position.Z));
	return MainOctree->GetLeaf(Position);
}

FChunkOctree* FVoxelRender::GetAdjacentChunk(EDirection Direction, const FIntVector& Position, int Size) const
{
	const int S = Size;
	TArray<FIntVector> L = {
		FIntVector(-S, 0, 0),
		FIntVector(+S, 0, 0),
		FIntVector(0, -S, 0),
		FIntVector(0, +S, 0),
		FIntVector(0, 0, -S),
		FIntVector(0, 0, +S)
	};

	FIntVector P = Position + L[Direction];

	if (Data->IsInWorld(P.X, P.Y, P.Z))
	{
		return GetChunkOctreeAt(P);
	}
	else
	{
		return nullptr;
	}
}

int FVoxelRender::GetDepthAt(const FIntVector& Position) const
{
	return GetChunkOctreeAt(Position)->Depth;
}

void FVoxelRender::Destroy()
{
	for (auto Chunk : ActiveChunks)
	{
		if (!Chunk->IsPendingKill())
		{
			Chunk->Delete();
		}
		else
		{
			Chunk->ResetRender();
		}
	}

	MeshThreadPool->Destroy();
	FoliageThreadPool->Destroy();

	ActiveChunks.Empty();
	InactiveChunks.resize(0);
}

FVector FVoxelRender::GetGlobalPosition(const FIntVector& LocalPosition)
{
	return World->LocalToGlobal(LocalPosition) + ChunksParent->GetActorLocation() - World->GetActorLocation();
}

void FVoxelRender::RemoveChunkFromQueue(FChunkOctree* Chunk)
{
	ChunksToUpdate.Remove(Chunk);
}
