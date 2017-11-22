// Copyright 2017 Phyronnaz

#include "VoxelInvokerComponent.h"
#include "VoxelPrivate.h"
#include "VoxelWorld.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

UVoxelInvokerComponent::UVoxelInvokerComponent()
	: bNeedUpdate(true)
	, DistanceOffset(1000)
	, bDebugMultiplayer(false)
{
	PrimaryComponentTick.bCanEverTick = true;
}

bool UVoxelInvokerComponent::IsForPhysicsOnly()
{
	return Cast<APawn>(GetOwner()) && !Cast<APawn>(GetOwner())->IsLocallyControlled();
}

void UVoxelInvokerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bDebugMultiplayer && !IsForPhysicsOnly())
	{
		DrawDebugPoint(GetWorld(), GetOwner()->GetActorLocation(), 100, FColor::Red, false, DeltaTime * 1.1f, 0);
	}

	if (bNeedUpdate)
	{
		if (true || GetNetMode() < ENetMode::NM_Client || (Cast<APawn>(GetOwner()) && Cast<APawn>(GetOwner())->Role == ROLE_AutonomousProxy))
		{
			TArray<AActor*> FoundActors;
			UGameplayStatics::GetAllActorsOfClass((UObject*)GetOwner()->GetWorld(), AVoxelWorld::StaticClass(), FoundActors);

			if (FoundActors.Num() == 0)
			{
				UE_LOG(LogVoxel, Warning, TEXT("No world found"));
			}
			else
			{
				for (auto Actor : FoundActors)
				{
					((AVoxelWorld*)Actor)->AddInvoker(TWeakObjectPtr<UVoxelInvokerComponent>(this));
				}
			}
		}
		bNeedUpdate = false;
	}
}
