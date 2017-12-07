// Copyright 2017 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VoxelInvokerComponent.generated.h"

class AVoxelWorld;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class VOXEL_API UVoxelInvokerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float DistanceOffset;

	// Will show a red point if local player
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool bDebugMultiplayer;

	UVoxelInvokerComponent();

	// TODO: not working for non pawn
	bool IsForPhysicsOnly();

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	bool bNeedUpdate;
};
