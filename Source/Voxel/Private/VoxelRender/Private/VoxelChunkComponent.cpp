// Copyright 2017 Phyronnaz

#include "VoxelChunkComponent.h"
#include "VoxelPrivate.h"
#include "VoxelRender.h"
#include "ChunkOctree.h"
#include "VoxelPolygonizer.h"
#include "InstancedStaticMesh.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "AI/Navigation/NavigationSystem.h"

#include "Runtime/Launch/Resources/Version.h"

DECLARE_CYCLE_STAT(TEXT("VoxelChunk ~ SetProcMeshSection"), STAT_SetProcMeshSection, STATGROUP_Voxel);
DECLARE_CYCLE_STAT(TEXT("VoxelChunk ~ Update"), STAT_Update, STATGROUP_Voxel);
DECLARE_CYCLE_STAT(TEXT("VoxelChunk ~ Update ~ Update neighbors"), STAT_UpdateUpdateNeighbors, STATGROUP_Voxel);
DECLARE_CYCLE_STAT(TEXT("VoxelChunk ~ Update ~ Async"), STAT_UpdateAsync, STATGROUP_Voxel);
DECLARE_CYCLE_STAT(TEXT("VoxelChunk ~ Update ~ Sync"), STAT_UpdateSync, STATGROUP_Voxel);
DECLARE_CYCLE_STAT(TEXT("VoxelChunk ~ Check for transitions"), STAT_CheckTransitions, STATGROUP_Voxel);

// Sets default values
UVoxelChunkComponent::UVoxelChunkComponent()
	: Render(nullptr)
	, MeshBuilder(nullptr)
	, CurrentOctree(nullptr)
{
	bCastShadowAsTwoSided = true;
	bUseAsyncCooking = true;
	SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	Mobility = EComponentMobility::Movable;
	CompletedFoliageTaskCount.Reset();
}

UVoxelChunkComponent::~UVoxelChunkComponent()
{
	if (Render)
	{
		Render->ChunkHasBeenDestroyed(this);
	}
	DeleteTasks();
}

void UVoxelChunkComponent::Init(FChunkOctree* NewOctree)
{
	check(!Render);
	check(!MeshBuilder);
	check(!CurrentOctree);

	CurrentOctree = NewOctree;
	{
		FScopeLock Lock(&RenderLock);
		Render = CurrentOctree->Render;
	}
	Position = CurrentOctree->Position;
	Size = CurrentOctree->Size();

	bCookCollisions = CurrentOctree->Depth == 0 && Render->World->GetComputeExtendedCollisions();
	/*if (bCookCollisions)
	{
		SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
	else
	{
		SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}*/

	FIntVector NewPosition = CurrentOctree->GetMinimalCornerPosition();

	SetWorldLocation(Render->GetGlobalPosition(NewPosition));
	SetWorldScale3D(FVector::OneVector * Render->World->GetVoxelSize());

	// Needed because octree is only partially builded when Init is called
	Render->AddTransitionCheck(this);

	ChunkHasHigherRes.SetNumZeroed(6);
}

bool UVoxelChunkComponent::Update(bool bAsync)
{
	check(Render);

	SCOPE_CYCLE_COUNTER(STAT_Update);

	// Update ChunkHasHigherRes
	if (Render->World->GetComputeTransitions() && CurrentOctree->Depth != 0)
	{
		SCOPE_CYCLE_COUNTER(STAT_UpdateUpdateNeighbors);
		for (int i = 0; i < 6; i++)
		{
			EDirection Direction = (EDirection)i;
			FChunkOctree* Chunk = Render->GetAdjacentChunk(Direction, Position, Size);
			if (Chunk)
			{
				ChunkHasHigherRes[i] = Chunk->Depth < CurrentOctree->Depth;
			}
			else
			{
				// Chunks at the edges of the world
				ChunkHasHigherRes[i] = false;
			}
		}
	}

	if (bAsync)
	{
		SCOPE_CYCLE_COUNTER(STAT_UpdateAsync);
		FScopeLock Lock(&MeshBuilderLock);
		if (!MeshBuilder)
		{
			MeshBuilder = new FAsyncPolygonizerTask(this);
			Render->MeshThreadPool->AddQueuedWork(MeshBuilder);

			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		SCOPE_CYCLE_COUNTER(STAT_UpdateSync);
		{
			FScopeLock Lock(&MeshBuilderLock);
			if (MeshBuilder)
			{
				MeshBuilder->DontDoCallback.Increment();
				MeshBuilder = nullptr;
			}
		}

		FVoxelPolygonizer* Builder = CreatePolygonizer();
		Builder->CreateSection(Section);
		delete Builder;

		ApplyNewMesh();

		return true;
	}
}

void UVoxelChunkComponent::CheckTransitions()
{
	SCOPE_CYCLE_COUNTER(STAT_CheckTransitions);
	// Cannot use CurrentOctree here

	check(Render);

	if (Render->World->GetComputeTransitions())
	{
		const int Depth = Render->GetDepthAt(Position);
		for (int i = 0; i < 6; i++)
		{
			auto Direction = (EDirection)i;
			FChunkOctree* Chunk = Render->GetAdjacentChunk(Direction, Position, Size);
			if (Chunk)
			{
				bool bThisHasHigherRes = Chunk->Depth > Depth;

				check(Chunk->GetVoxelChunk());
				if (bThisHasHigherRes != Chunk->GetVoxelChunk()->HasChunkHigherRes(InvertDirection(Direction)))
				{
					Render->UpdateChunk(Chunk, true);
				}
			}
		}
	}
}

void UVoxelChunkComponent::Unload()
{
	check(Render);

	DeleteTasks();

	Render->AddTransitionCheck(this); // Needed because octree is only partially updated when Unload is called
	Render->ScheduleDeletion(this);

	CurrentOctree = nullptr;
}

void UVoxelChunkComponent::Delete()
{
	check(Render);
	{
		FScopeLock Lock(&RenderLock);
		Render = nullptr;
	}

	DeleteTasks();

	// Reset mesh
	SetProcMeshSection(0, FVoxelProcMeshSection());

	// Delete foliage
	for (auto FoliageComponent : FoliageComponents)
	{
		FoliageComponent->DestroyComponent();
	}
	FoliageComponents.Empty();

	CurrentOctree = nullptr;
}

void UVoxelChunkComponent::OnMeshComplete(const FVoxelProcMeshSection& InSection, FAsyncPolygonizerTask* InTask)
{
	bool bRenderIsValid;
	{
		FScopeLock Lock(&RenderLock);
		bRenderIsValid = Render != nullptr;
	}

	if (bRenderIsValid)
	{
		bool bSame;
		{
			FScopeLock Lock(&MeshBuilderLock);
			bSame = (MeshBuilder == InTask);
		}
		if (bSame)
		{
			{
				FScopeLock Lock(&MeshBuilderLock);
				MeshBuilder = nullptr;
			}
			Section = InSection; // May be slow

			{
				FScopeLock Lock(&RenderLock);
				Render->AddApplyNewMesh(this);
			}
		}
		else
		{
			UE_LOG(LogVoxel, Warning, TEXT("Invalid FAsyncPolygonizerTask"));
		}
	}
}

void UVoxelChunkComponent::ApplyNewMesh()
{
	SCOPE_CYCLE_COUNTER(STAT_SetProcMeshSection);

	check(Render);

	Render->AddFoliageUpdate(this);

	SetProcMeshSection(0, Section);

	if (CurrentOctree->Depth == 0)
	{
		UNavigationSystem::UpdateComponentInNavOctree(*this);
	}
}

void UVoxelChunkComponent::SetVoxelMaterial(UMaterialInterface* Material)
{
	SetMaterial(0, Material);
}

bool UVoxelChunkComponent::HasChunkHigherRes(EDirection Direction)
{
	return CurrentOctree->Depth != 0 && ChunkHasHigherRes[Direction];
}

bool UVoxelChunkComponent::UpdateFoliage()
{
	check(Render);

	if (FoliageTasks.Num() == 0)
	{
		CompletedFoliageTaskCount.Reset();

		int GrassVarietyIndex = 0;
		for (int Index = 0; Index < Render->World->GrassTypes.Num(); Index++)
		{
			if (Render->World->GrassTypes[Index])
			{
				auto GrassType = Render->World->GrassTypes[Index];
				for (auto GrassVariety : GrassType->GrassVarieties)
				{
					if (GrassVariety.CullDepth >= CurrentOctree->Depth)
					{
						FAsyncTask<FAsyncFoliageTask>* FoliageTask = new FAsyncTask<FAsyncFoliageTask>(
							Section
							, GrassVariety
							, GrassVarietyIndex
							, Index
							, Render->World
							, CurrentOctree->GetMinimalCornerPosition()
							, this);
						GrassVarietyIndex++;

						FoliageTask->StartBackgroundTask(Render->FoliageThreadPool);
						FoliageTasks.Add(FoliageTask);
					}
				}
			}
		}
		return true;
	}
	else
	{
		return false;
	}
}


void UVoxelChunkComponent::OnFoliageComplete()
{
	CompletedFoliageTaskCount.Increment();
	if (CompletedFoliageTaskCount.GetValue() == FoliageTasks.Num())
	{
		OnAllFoliageComplete();
	}
}

void UVoxelChunkComponent::OnAllFoliageComplete()
{
	check(Render);

	Render->AddApplyNewFoliage(this);
	CompletedFoliageTaskCount.Reset();
}

void UVoxelChunkComponent::ApplyNewFoliage()
{
	check(Render);

	for (int i = 0; i < FoliageComponents.Num(); i++)
	{
		FoliageComponents[i]->DestroyComponent();
	}
	FoliageComponents.Empty();

	for (auto FoliageTask : FoliageTasks)
	{
		FoliageTask->EnsureCompletion();
		FAsyncFoliageTask Task = FoliageTask->GetTask();
		if (Task.InstanceBuffer.NumInstances())
		{
			FVoxelGrassVariety GrassVariety = Task.GrassVariety;

			int32 FolSeed = FCrc::StrCrc32((GrassVariety.GrassMesh->GetName() + GetName()).GetCharArray().GetData());
			if (FolSeed == 0)
			{
				FolSeed++;
			}

			//Create component
			UHierarchicalInstancedStaticMeshComponent* HierarchicalInstancedStaticMeshComponent = NewObject<UHierarchicalInstancedStaticMeshComponent>(Render->ChunksParent, NAME_None, RF_Transient);

			HierarchicalInstancedStaticMeshComponent->OnComponentCreated();
			HierarchicalInstancedStaticMeshComponent->RegisterComponent();
			if (HierarchicalInstancedStaticMeshComponent->bWantsInitializeComponent) HierarchicalInstancedStaticMeshComponent->InitializeComponent();

			HierarchicalInstancedStaticMeshComponent->Mobility = EComponentMobility::Movable;
			HierarchicalInstancedStaticMeshComponent->bCastStaticShadow = false;

			HierarchicalInstancedStaticMeshComponent->SetStaticMesh(GrassVariety.GrassMesh);
			HierarchicalInstancedStaticMeshComponent->MinLOD = GrassVariety.MinLOD;
			HierarchicalInstancedStaticMeshComponent->bSelectable = false;
			HierarchicalInstancedStaticMeshComponent->bHasPerInstanceHitProxies = false;
			HierarchicalInstancedStaticMeshComponent->bReceivesDecals = GrassVariety.bReceivesDecals;
			static FName NoCollision(TEXT("NoCollision"));
			HierarchicalInstancedStaticMeshComponent->SetCollisionProfileName(NoCollision);
			HierarchicalInstancedStaticMeshComponent->bDisableCollision = true;
			HierarchicalInstancedStaticMeshComponent->SetCanEverAffectNavigation(false);
			HierarchicalInstancedStaticMeshComponent->InstancingRandomSeed = FolSeed;
			HierarchicalInstancedStaticMeshComponent->LightingChannels = GrassVariety.LightingChannels;

			HierarchicalInstancedStaticMeshComponent->InstanceStartCullDistance = GrassVariety.StartCullDistance;
			HierarchicalInstancedStaticMeshComponent->InstanceEndCullDistance = GrassVariety.EndCullDistance;

			HierarchicalInstancedStaticMeshComponent->bAffectDistanceFieldLighting = false;

			HierarchicalInstancedStaticMeshComponent->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
			FTransform DesiredTransform = this->GetComponentTransform();
			DesiredTransform.RemoveScaling();
			HierarchicalInstancedStaticMeshComponent->SetWorldTransform(DesiredTransform);

			FoliageComponents.Add(HierarchicalInstancedStaticMeshComponent);

#if ENGINE_MINOR_VERSION == 17
			if (!HierarchicalInstancedStaticMeshComponent->PerInstanceRenderData.IsValid())
			{
				HierarchicalInstancedStaticMeshComponent->InitPerInstanceRenderData(&Task.InstanceBuffer);
			}
			else
			{
				HierarchicalInstancedStaticMeshComponent->PerInstanceRenderData->UpdateFromPreallocatedData(HierarchicalInstancedStaticMeshComponent, Task.InstanceBuffer);
			}
#else
			if (!HierarchicalInstancedStaticMeshComponent->PerInstanceRenderData.IsValid())
			{
				HierarchicalInstancedStaticMeshComponent->InitPerInstanceRenderData(true, &Task.InstanceBuffer);
			}
			else
			{
				HierarchicalInstancedStaticMeshComponent->PerInstanceRenderData->UpdateFromPreallocatedData(HierarchicalInstancedStaticMeshComponent, Task.InstanceBuffer, false);
			}
#endif


			HierarchicalInstancedStaticMeshComponent->AcceptPrebuiltTree(Task.ClusterTree, Task.OutOcclusionLayerNum);

			//HierarchicalInstancedStaticMeshComponent->RecreateRenderState_Concurrent();
			HierarchicalInstancedStaticMeshComponent->MarkRenderStateDirty();
		}

		delete FoliageTask;
	}
	FoliageTasks.Empty();
}


void UVoxelChunkComponent::ResetRender()
{
	FScopeLock Lock(&RenderLock);
	Render = nullptr;
}

void UVoxelChunkComponent::Serialize(FArchive& Ar)
{

}

void UVoxelChunkComponent::DeleteTasks()
{
	{
		FScopeLock Lock(&MeshBuilderLock);
		if (MeshBuilder)
		{
			if (Render && Render->MeshThreadPool->RetractQueuedWork(MeshBuilder))
			{
				delete MeshBuilder;
			}
			else
			{
				MeshBuilder->DontDoCallback.Increment();
			}
			MeshBuilder = nullptr;
		}
	}
	for (auto FoliageTask : FoliageTasks)
	{
		if (!FoliageTask->Cancel())
		{
			FoliageTask->EnsureCompletion();
		}
		check(FoliageTask->IsDone());
		delete FoliageTask;
	}
	FoliageTasks.Empty();
	CompletedFoliageTaskCount.Reset();
}

FVoxelPolygonizer* UVoxelChunkComponent::CreatePolygonizer(FAsyncPolygonizerTask* Task)
{
	FScopeLock Lock(&MeshBuilderLock);
	FScopeLock LockRender(&RenderLock);

	if (Render && CurrentOctree && (!Task || (MeshBuilder && Task == MeshBuilder)))
	{
		return new FVoxelPolygonizer(
			CurrentOctree->Depth,
			Render->Data,
			CurrentOctree->GetMinimalCornerPosition(),
			ChunkHasHigherRes,
			CurrentOctree->Depth != 0 && Render->World->GetComputeTransitions(),
			CurrentOctree->Depth <= Render->World->GetMaxDepthToGenerateCollisions() && Render->World->GetComputeExtendedCollisions(),
			Render->World->GetEnableAmbientOcclusion(),
			Render->World->GetRayMaxDistance(),
			Render->World->GetRayCount(),
			Render->World->GetNormalThresholdForSimplification()
		);
	}
	else
	{
		return nullptr;
	}
}
