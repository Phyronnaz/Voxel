// Copyright 2017 Phyronnaz

#include "VoxelRender.h"
#include "VoxelPrivatePCH.h"
#include "VoxelChunkComponent.h"
#include "ChunkOctree.h"
#include "ProceduralMeshComponent.h"
#include "VoxelPolygonizerForCollisions.h"
#include "VoxelInvokerComponent.h"

DECLARE_CYCLE_STAT(TEXT("VoxelRender ~ ApplyUpdates"), STAT_ApplyUpdates, STATGROUP_Voxel);
DECLARE_CYCLE_STAT(TEXT("VoxelRender ~ UpdateLOD"), STAT_UpdateLOD, STATGROUP_Voxel);

class FCollisionMeshHandler
{
public:
	TWeakObjectPtr<UVoxelInvokerComponent> Invoker;
	AVoxelWorld* const World;

	FCollisionMeshHandler(TWeakObjectPtr<UVoxelInvokerComponent> Invoker, AVoxelWorld* const World)
		: Invoker(Invoker)
		, World(World)
	{
		check(Invoker.IsValid());
		CurrentCenter = World->GlobalToLocal(Invoker.Get()->GetOwner()->GetActorLocation());

		for (int i = 0; i < 2; i++)
		{
			for (int j = 0; j < 2; j++)
			{
				for (int k = 0; k < 2; k++)
				{
					UProceduralMeshComponent* Chunk = NewObject<UProceduralMeshComponent>(World, NAME_None, RF_Transient | RF_NonPIEDuplicateTransient);
					Chunk->bUseAsyncCooking = true;
					Chunk->SetupAttachment(World->GetRootComponent(), NAME_None);
					Chunk->RegisterComponent();
					Chunk->SetWorldScale3D(FVector::OneVector * World->GetVoxelSize());
					Chunk->SetWorldLocation(World->LocalToGlobal(CurrentCenter + FIntVector(i - 1, j - 1, k - 1) * CHUNKSIZE_FC));
					Components[i][j][k] = Chunk;
					Update(i, j, k);
				}
			}
		}
	}

	void UpdateComponentsForNewCenter()
	{
		if (!Invoker.IsValid())
		{
			UE_LOG(VoxelLog, Error, TEXT("Invalid invoker"));
			return;
		}
		const FIntVector NewPosition = World->GlobalToLocal(Invoker.Get()->GetOwner()->GetActorLocation());
		const FIntVector Delta = NewPosition - CurrentCenter;

		if (Delta.X > CHUNKSIZE_FC / 2)
		{
			UProceduralMeshComponent* tmp[2][2];

			tmp[0][0] = Components[0][0][0];
			tmp[0][1] = Components[0][0][1];
			tmp[1][0] = Components[0][1][0];
			tmp[1][1] = Components[0][1][1];

			Components[0][0][0] = Components[1][0][0];
			Components[0][0][1] = Components[1][0][1];
			Components[0][1][0] = Components[1][1][0];
			Components[0][1][1] = Components[1][1][1];

			Components[1][0][0] = tmp[0][0];
			Components[1][0][1] = tmp[0][1];
			Components[1][1][0] = tmp[1][0];
			Components[1][1][1] = tmp[1][1];

			CurrentCenter += FIntVector(CHUNKSIZE_FC, 0, 0);

			ChunksToUpdate.Add(FIntVector(true, false, false));
			ChunksToUpdate.Add(FIntVector(true, true, false));
			ChunksToUpdate.Add(FIntVector(true, false, true));
			ChunksToUpdate.Add(FIntVector(true, true, true));
		}
		else if (-Delta.X > CHUNKSIZE_FC / 2)
		{
			UProceduralMeshComponent* tmp[2][2];

			tmp[0][0] = Components[1][0][0];
			tmp[0][1] = Components[1][0][1];
			tmp[1][0] = Components[1][1][0];
			tmp[1][1] = Components[1][1][1];

			Components[1][0][0] = Components[0][0][0];
			Components[1][0][1] = Components[0][0][1];
			Components[1][1][0] = Components[0][1][0];
			Components[1][1][1] = Components[0][1][1];

			Components[0][0][0] = tmp[0][0];
			Components[0][0][1] = tmp[0][1];
			Components[0][1][0] = tmp[1][0];
			Components[0][1][1] = tmp[1][1];

			CurrentCenter -= FIntVector(CHUNKSIZE_FC, 0, 0);

			ChunksToUpdate.Add(FIntVector(false, false, false));
			ChunksToUpdate.Add(FIntVector(false, true, false));
			ChunksToUpdate.Add(FIntVector(false, false, true));
			ChunksToUpdate.Add(FIntVector(false, true, true));
		}
		if (Delta.Y > CHUNKSIZE_FC / 2)
		{
			UProceduralMeshComponent* tmp[2][2];

			tmp[0][0] = Components[0][0][0];
			tmp[0][1] = Components[0][0][1];
			tmp[1][0] = Components[1][0][0];
			tmp[1][1] = Components[1][0][1];

			Components[0][0][0] = Components[0][1][0];
			Components[0][0][1] = Components[0][1][1];
			Components[1][0][0] = Components[1][1][0];
			Components[1][0][1] = Components[1][1][1];

			Components[0][1][0] = tmp[0][0];
			Components[0][1][1] = tmp[0][1];
			Components[1][1][0] = tmp[1][0];
			Components[1][1][1] = tmp[1][1];

			CurrentCenter += FIntVector(0, CHUNKSIZE_FC, 0);

			ChunksToUpdate.Add(FIntVector(false, true, false));
			ChunksToUpdate.Add(FIntVector(true, true, false));
			ChunksToUpdate.Add(FIntVector(false, true, true));
			ChunksToUpdate.Add(FIntVector(true, true, true));
		}
		else if (-Delta.Y > CHUNKSIZE_FC / 2)
		{
			UProceduralMeshComponent* tmp[2][2];

			tmp[0][0] = Components[0][1][0];
			tmp[0][1] = Components[0][1][1];
			tmp[1][0] = Components[1][1][0];
			tmp[1][1] = Components[1][1][1];

			Components[0][1][0] = Components[0][0][0];
			Components[0][1][1] = Components[0][0][1];
			Components[1][1][0] = Components[1][0][0];
			Components[1][1][1] = Components[1][0][1];

			Components[0][0][0] = tmp[0][0];
			Components[0][0][1] = tmp[0][1];
			Components[1][0][0] = tmp[1][0];
			Components[1][0][1] = tmp[1][1];

			CurrentCenter -= FIntVector(0, CHUNKSIZE_FC, 0);

			ChunksToUpdate.Add(FIntVector(false, false, false));
			ChunksToUpdate.Add(FIntVector(true, false, false));
			ChunksToUpdate.Add(FIntVector(false, false, true));
			ChunksToUpdate.Add(FIntVector(true, false, true));
		}
		if (Delta.Z > CHUNKSIZE_FC / 2)
		{
			UProceduralMeshComponent* tmp[2][2];

			tmp[0][0] = Components[0][0][0];
			tmp[0][1] = Components[0][1][0];
			tmp[1][0] = Components[1][0][0];
			tmp[1][1] = Components[1][1][0];

			Components[0][0][0] = Components[0][0][1];
			Components[0][1][0] = Components[0][1][1];
			Components[1][0][0] = Components[1][0][1];
			Components[1][1][0] = Components[1][1][1];

			Components[0][0][1] = tmp[0][0];
			Components[0][1][1] = tmp[0][1];
			Components[1][0][1] = tmp[1][0];
			Components[1][1][1] = tmp[1][1];

			CurrentCenter += FIntVector(0, 0, CHUNKSIZE_FC);

			ChunksToUpdate.Add(FIntVector(false, false, true));
			ChunksToUpdate.Add(FIntVector(true, false, true));
			ChunksToUpdate.Add(FIntVector(false, true, true));
			ChunksToUpdate.Add(FIntVector(true, true, true));
		}
		else if (-Delta.Z > CHUNKSIZE_FC / 2)
		{
			UProceduralMeshComponent* tmp[2][2];

			tmp[0][0] = Components[0][0][1];
			tmp[0][1] = Components[0][1][1];
			tmp[1][0] = Components[1][0][1];
			tmp[1][1] = Components[1][1][1];

			Components[0][0][1] = Components[0][0][0];
			Components[0][1][1] = Components[0][1][0];
			Components[1][0][1] = Components[1][0][0];
			Components[1][1][1] = Components[1][1][0];

			Components[0][0][0] = tmp[0][0];
			Components[0][1][0] = tmp[0][1];
			Components[1][0][0] = tmp[1][0];
			Components[1][1][0] = tmp[1][1];

			CurrentCenter -= FIntVector(0, 0, CHUNKSIZE_FC);

			ChunksToUpdate.Add(FIntVector(false, false, false));
			ChunksToUpdate.Add(FIntVector(true, false, false));
			ChunksToUpdate.Add(FIntVector(false, true, false));
			ChunksToUpdate.Add(FIntVector(true, true, false));
		}

		for (auto P : ChunksToUpdate)
		{
			Update(P.X, P.Y, P.Z);
		}
		ChunksToUpdate.Reset();
	}

	void Update(bool bXMax, bool bYMax, bool bZMax)
	{
		const FIntVector ChunkPosition = CurrentCenter + FIntVector(bXMax - 1, bYMax - 1, bZMax - 1) * CHUNKSIZE_FC;

		FVoxelPolygonizerForCollisions Poly(World->GetData(), ChunkPosition);
		FProcMeshSection Section;
		Poly.CreateSection(Section);

		Components[bXMax][bYMax][bZMax]->SetProcMeshSection(0, Section);
		Components[bXMax][bYMax][bZMax]->SetWorldLocation(World->LocalToGlobal(ChunkPosition), false, nullptr, ETeleportType::None);
	}

	bool IsValid()
	{
		return Invoker.IsValid();
	};

	void Destroy()
	{
		for (int i = 0; i < 2; i++)
		{
			for (int j = 0; j < 2; j++)
			{
				for (int k = 0; k < 2; k++)
				{
					Components[i][j][k]->DestroyComponent();
				}
			}
		}
	}

	void UpdateInBox(FVoxelBox Box)
	{
		for (int i = 0; i < 2; i++)
		{
			for (int j = 0; j < 2; j++)
			{
				for (int k = 0; k < 2; k++)
				{
					FIntVector P(i - 1, j - 1, k - 1);
					FVoxelBox CurrentBox(P * CHUNKSIZE_FC + CurrentCenter, (P + FIntVector(1, 1, 1)) * CHUNKSIZE_FC + CurrentCenter);
					if (CurrentBox.Intersect(Box))
					{
						ChunksToUpdate.Add(FIntVector(i, j, k));
					}
				}
			}
		}
	}

private:
	FIntVector CurrentCenter;
	UProceduralMeshComponent* Components[2][2][2];
	TSet<FIntVector> ChunksToUpdate;
};

FVoxelRender::FVoxelRender(AVoxelWorld* World, AActor* ChunksParent, FVoxelData* Data, uint32 MeshThreadCount, uint32 HighPriorityMeshThreadCount, uint32 FoliageThreadCount)
	: World(World)
	, ChunksParent(ChunksParent)
	, Data(Data)
	, MeshThreadPool(FQueuedThreadPool::Allocate())
	, FoliageThreadPool(FQueuedThreadPool::Allocate())
	, TimeSinceFoliageUpdate(0)
	, TimeSinceLODUpdate(0)
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
	for (FChunkToDelete& ChunkToDelete : ChunksToDelete)
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
	ChunksToDelete.remove_if([](FChunkToDelete ChunkToDelete) { return ChunkToDelete.TimeLeft < 0; });
}

void FVoxelRender::AddInvoker(TWeakObjectPtr<UVoxelInvokerComponent> Invoker)
{
	VoxelInvokerComponents.push_front(Invoker);
	CollisionComponents.Add(new FCollisionMeshHandler(Invoker, World));
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

void FVoxelRender::UpdateChunk(TWeakPtr<FChunkOctree> Chunk, bool bAsync)
{
	if (Chunk.IsValid())
	{
		ChunksToUpdate.Add(Chunk);
		if (!bAsync)
		{
			IdsOfChunksToUpdateSynchronously.Add(Chunk.Pin().Get()->Id);
		}
	}
}

void FVoxelRender::UpdateChunksAtPosition(FIntVector Position, bool bAsync)
{
	check(Data->IsInWorld(Position.X, Position.Y, Position.Z));

	UpdateChunksOverlappingBox(FVoxelBox(Position, Position), bAsync);
}

void FVoxelRender::UpdateChunksOverlappingBox(FVoxelBox Box, bool bAsync)
{
	Box.Min -= FIntVector(2, 2, 2); // For normals
	Box.Max += FIntVector(2, 2, 2); // For normals
	std::forward_list<TWeakPtr<FChunkOctree>> OverlappingLeafs;
	MainOctree->GetLeafsOverlappingBox(Box, OverlappingLeafs);

	for (auto Chunk : OverlappingLeafs)
	{
		UpdateChunk(Chunk, bAsync);
	}

	for (auto& Handler : CollisionComponents)
	{
		if (Handler->IsValid())
		{
			Handler->UpdateInBox(Box);
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

	std::forward_list<TWeakPtr<FChunkOctree>> Failed;

	for (auto& Chunk : ChunksToUpdate)
	{
		TSharedPtr<FChunkOctree> LockedChunk(Chunk.Pin());

		if (LockedChunk.IsValid() && LockedChunk->GetVoxelChunk())
		{
			bool bAsync = !IdsOfChunksToUpdateSynchronously.Contains(LockedChunk->Id);
			bool bSuccess = LockedChunk->GetVoxelChunk()->Update(bAsync);

			/*if (!bSuccess)
			{
				UE_LOG(VoxelLog, Warning, TEXT("Chunk already updating"));
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
	std::forward_list<TWeakObjectPtr<UVoxelInvokerComponent>> Temp;
	for (auto Invoker : VoxelInvokerComponents)
	{
		if (Invoker.IsValid())
		{
			Temp.push_front(Invoker);
		}
	}
	VoxelInvokerComponents = Temp;

	for (auto& Handler : CollisionComponents)
	{
		if (Handler->IsValid())
		{
			Handler->UpdateComponentsForNewCenter();
		}
		else
		{
			Handler->Destroy();
			delete Handler;
			Handler = nullptr;
		}
	}

	CollisionComponents.RemoveAll([](void* P) { return P == nullptr; });

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

	ChunksToDelete.remove_if([Chunk](FChunkToDelete ChunkToDelete) { return ChunkToDelete.Chunk == Chunk; });

	InactiveChunks.remove(Chunk);
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

TWeakPtr<FChunkOctree> FVoxelRender::GetChunkOctreeAt(FIntVector Position) const
{
	check(Data->IsInWorld(Position.X, Position.Y, Position.Z));
	return MainOctree->GetLeaf(Position);
}

int FVoxelRender::GetDepthAt(FIntVector Position) const
{
	return GetChunkOctreeAt(Position).Pin()->Depth;
}

void FVoxelRender::Destroy()
{
	MeshThreadPool->Destroy();
	FoliageThreadPool->Destroy();

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

	ActiveChunks.Empty();
	InactiveChunks.resize(0);
}

FVector FVoxelRender::GetGlobalPosition(FIntVector LocalPosition)
{
	return World->LocalToGlobal(LocalPosition) + ChunksParent->GetActorLocation() - World->GetActorLocation();
}
