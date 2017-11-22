// Copyright 2017 Phyronnaz

#include "VoxelAutoDisableComponent.h"
#include "VoxelPrivate.h"
#include "VoxelWorld.h"
#include "Kismet/GameplayStatics.h"

DECLARE_CYCLE_STAT(TEXT("VoxelAutoDisableComponent ~ Tick"), STAT_VoxelAutoDisable_Tick, STATGROUP_Voxel);

UVoxelAutoDisableComponent::UVoxelAutoDisableComponent() : Component(nullptr), bSimulatePhysics(false), bAutoFindWorld(true), World(nullptr), CullDepth(4), bAutoFindComponent(true)
{
	PrimaryComponentTick.bCanEverTick = true;
}


void UVoxelAutoDisableComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!Component && bAutoFindComponent)
	{
		Component = Cast<UPrimitiveComponent>(GetOwner()->GetComponentByClass(TSubclassOf<UActorComponent>(ComponentClass)));

		if (!Component)
		{
			UE_LOG(LogVoxel, Error, TEXT("VoxelAutoDisableComponent: Invalid component class"));
		}
		else
		{
			bSimulatePhysics = Component->IsSimulatingPhysics();
		}
	}
}


void UVoxelAutoDisableComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	SCOPE_CYCLE_COUNTER(STAT_VoxelAutoDisable_Tick);

	if (Component)
	{
		if (!World && bAutoFindWorld)
		{
			TArray<AActor*> FoundActors;
			UGameplayStatics::GetAllActorsOfClass((UObject*)GetOwner()->GetWorld(), AVoxelWorld::StaticClass(), FoundActors);

			if (FoundActors.Num() == 0)
			{
				UE_LOG(LogVoxel, Warning, TEXT("No world found"));
			}
			else
			{
				if (FoundActors.Num() > 1)
				{
					UE_LOG(LogVoxel, Warning, TEXT("More than one world found"));
				}
				World = (AVoxelWorld*)FoundActors[0];
			}
		}

		if (World)
		{
			FIntVector LocalPosition = World->GlobalToLocal(GetOwner()->GetActorLocation());
			if (World->IsInWorld(LocalPosition))
			{
				int Depth = World->GetDepthAt(LocalPosition);
				if (bSimulatePhysics)
				{
					Component->SetSimulatePhysics(Depth == 0);
				}

				Component->SetVisibility(Depth <= CullDepth, true);
			}
		}
	}
}

