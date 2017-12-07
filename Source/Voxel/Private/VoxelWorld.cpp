// Copyright 2017 Phyronnaz

#include "VoxelWorld.h"
#include "VoxelPrivate.h"
#include "VoxelData.h"
#include "VoxelRender.h"
#include "Components/CapsuleComponent.h"
#include "FlatWorldGenerator.h"
#include "VoxelInvokerComponent.h"
#include "VoxelWorldEditorInterface.h"
#include <deque>

AVoxelWorld::AVoxelWorld()
	: VoxelWorldEditorClass(nullptr)
	, NewDepth(9)
	, DeletionDelay(0.1f)
	, bComputeTransitions(true)
	, bIsCreated(false)
	, FoliageFPS(15)
	, LODUpdateFPS(10)
	, CollisionUpdateFPS(30)
	, NewVoxelSize(100)
	, Seed(100)
	, MeshThreadCount(4)
	, FoliageThreadCount(4)
	, Render(nullptr)
	, Data(nullptr)
	, InstancedWorldGenerator(nullptr)
	, VoxelWorldEditor(nullptr)
	, bComputeCollisions(false)
	, bComputeExtendedCollisions(true)
	, bDebugCollisions(false)
	, NormalThresholdForSimplification(0.1f)
	, bEnableAmbientOcclusion(false)
	, RayMaxDistance(5)
	, RayCount(25)
	, TimeSinceSync(0)
{
	PrimaryActorTick.bCanEverTick = true;

	auto TouchCapsule = CreateDefaultSubobject<UCapsuleComponent>(FName("Capsule"));
	TouchCapsule->InitCapsuleSize(0.1f, 0.1f);
	TouchCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TouchCapsule->SetCollisionResponseToAllChannels(ECR_Ignore);
	RootComponent = TouchCapsule;

	WorldGenerator = TSubclassOf<UVoxelWorldGenerator>(UFlatWorldGenerator::StaticClass());
}

AVoxelWorld::~AVoxelWorld()
{
	if (Data)
	{
		delete Data;
	}
	if (Render)
	{
		Render->Destroy();
		delete Render;
	}
}

void AVoxelWorld::BeginPlay()
{
	Super::BeginPlay();

	if (!IsCreated())
	{
		CreateWorld();
	}

	bComputeCollisions = true;
}

void AVoxelWorld::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsCreated())
	{
		Render->Tick(DeltaTime);
	}
}

void AVoxelWorld::BeginDestroy()
{
	Super::BeginDestroy();

	if (Render)
	{
		Render->Destroy();
		delete Render;
		Render = nullptr;
	}
}

#if WITH_EDITOR
bool AVoxelWorld::ShouldTickIfViewportsOnly() const
{
	return true;
}

void AVoxelWorld::PostLoad()
{
	Super::PostLoad();

	if (GetWorld())
	{
		CreateInEditor();
	}
}

#endif

float AVoxelWorld::GetValue(const FIntVector& Position) const
{
	if (IsInWorld(Position))
	{
		FVoxelMaterial Material;
		float Value;

		Data->BeginGet();
		Data->GetValueAndMaterial(Position.X, Position.Y, Position.Z, Value, Material);
		Data->EndGet();

		return Value;
	}
	else
	{
		UE_LOG(LogVoxel, Error, TEXT("Get value: Not in world: (%d, %d, %d)"), Position.X, Position.Y, Position.Z);
		return 0;
	}
}

FVoxelMaterial AVoxelWorld::GetMaterial(const FIntVector& Position) const
{
	if (IsInWorld(Position))
	{
		FVoxelMaterial Material;
		float Value;

		Data->BeginGet();
		Data->GetValueAndMaterial(Position.X, Position.Y, Position.Z, Value, Material);
		Data->EndGet();

		return Material;
	}
	else
	{
		UE_LOG(LogVoxel, Error, TEXT("Get material: Not in world: (%d, %d, %d)"), Position.X, Position.Y, Position.Z);
		return FVoxelMaterial();
	}
}

void AVoxelWorld::SetValue(const FIntVector& Position, float Value)
{
	if (IsInWorld(Position))
	{
		Data->BeginSet();
		Data->SetValue(Position.X, Position.Y, Position.Z, Value);
		Data->EndSet();
	}
	else
	{
		UE_LOG(LogVoxel, Error, TEXT("Get material: Not in world: (%d, %d, %d)"), Position.X, Position.Y, Position.Z);
	}
}

void AVoxelWorld::SetMaterial(const FIntVector& Position, const FVoxelMaterial& Material)
{
	if (IsInWorld(Position))
	{
		Data->BeginSet();
		Data->SetMaterial(Position.X, Position.Y, Position.Z, Material);
		Data->EndSet();
	}
	else
	{
		UE_LOG(LogVoxel, Error, TEXT("Set material: Not in world: (%d, %d, %d)"), Position.X, Position.Y, Position.Z);
	}
}

void AVoxelWorld::GetSave(FVoxelWorldSave& OutSave) const
{
	Data->GetSave(OutSave);
}

void AVoxelWorld::LoadFromSave(const FVoxelWorldSave& Save, bool bReset)
{
	if (Save.Depth == Depth)
	{
		std::deque<FIntVector> ModifiedPositions;
		Data->LoadFromSaveAndGetModifiedPositions(Save, ModifiedPositions, bReset);
		for (auto Position : ModifiedPositions)
		{
			if (IsInWorld(Position))
			{
				UpdateChunksAtPosition(Position, true);
			}
		}
		Render->ApplyUpdates();
	}
	else
	{
		UE_LOG(LogVoxel, Error, TEXT("LoadFromSave: Current Depth is %d while Save one is %d"), Depth, Save.Depth);
	}
}

AVoxelWorldEditorInterface* AVoxelWorld::GetVoxelWorldEditor() const
{
	return VoxelWorldEditor;
}

FVoxelData* AVoxelWorld::GetData() const
{
	return Data;
}

UVoxelWorldGenerator* AVoxelWorld::GetWorldGenerator() const
{
	return InstancedWorldGenerator;
}

int32 AVoxelWorld::GetSeed() const
{
	return Seed;
}

float AVoxelWorld::GetFoliageFPS() const
{
	return FoliageFPS;
}

float AVoxelWorld::GetLODUpdateFPS() const
{
	return LODUpdateFPS;
}

float AVoxelWorld::GetCollisionUpdateFPS() const
{
	return CollisionUpdateFPS;
}

UMaterialInterface* AVoxelWorld::GetVoxelMaterial() const
{
	return VoxelMaterial;
}

bool AVoxelWorld::GetComputeTransitions() const
{
	return bComputeTransitions;
}

bool AVoxelWorld::GetComputeCollisions() const
{
	return bComputeCollisions;
}

bool AVoxelWorld::GetComputeExtendedCollisions()
{
	return bComputeExtendedCollisions;
}

uint8 AVoxelWorld::GetMaxDepthToGenerateCollisions() const
{
	return MaxDepthToGenerateCollisions;
}

bool AVoxelWorld::GetDebugCollisions() const
{
	return bDebugCollisions;
}

float AVoxelWorld::GetDeletionDelay() const
{
	return DeletionDelay;
}

float AVoxelWorld::GetNormalThresholdForSimplification() const
{
	return NormalThresholdForSimplification;
}

bool AVoxelWorld::GetEnableAmbientOcclusion() const
{
	return bEnableAmbientOcclusion;
}

int AVoxelWorld::GetRayMaxDistance() const
{
	return RayMaxDistance;
}

int AVoxelWorld::GetRayCount() const
{
	return RayCount;
}

FIntVector AVoxelWorld::GlobalToLocal(const FVector& Position) const
{
	FVector P = GetTransform().InverseTransformPosition(Position) / GetVoxelSize();
	return FIntVector(FMath::RoundToInt(P.X), FMath::RoundToInt(P.Y), FMath::RoundToInt(P.Z));
}

FVector AVoxelWorld::LocalToGlobal(const FIntVector& Position) const
{
	return LocalToGlobal((FVector)Position);
}

FVector AVoxelWorld::LocalToGlobal(const FVector& Position) const
{
	return GetTransform().TransformPosition(GetVoxelSize() * Position);
}

TArray<FIntVector> AVoxelWorld::GetNeighboringPositions(const FVector& GlobalPosition) const
{
	FVector P = GetTransform().InverseTransformPosition(GlobalPosition) / GetVoxelSize();
	return TArray<FIntVector>({
		FIntVector(FMath::FloorToInt(P.X), FMath::FloorToInt(P.Y), FMath::FloorToInt(P.Z)),
		FIntVector(FMath::CeilToInt(P.X) , FMath::FloorToInt(P.Y), FMath::FloorToInt(P.Z)),
		FIntVector(FMath::FloorToInt(P.X), FMath::CeilToInt(P.Y) , FMath::FloorToInt(P.Z)),
		FIntVector(FMath::CeilToInt(P.X) , FMath::CeilToInt(P.Y) , FMath::FloorToInt(P.Z)),
		FIntVector(FMath::FloorToInt(P.X), FMath::FloorToInt(P.Y), FMath::CeilToInt(P.Z)),
		FIntVector(FMath::CeilToInt(P.X) , FMath::FloorToInt(P.Y), FMath::CeilToInt(P.Z)),
		FIntVector(FMath::FloorToInt(P.X), FMath::CeilToInt(P.Y) , FMath::CeilToInt(P.Z)),
		FIntVector(FMath::CeilToInt(P.X) , FMath::CeilToInt(P.Y) , FMath::CeilToInt(P.Z))
		});
}

void AVoxelWorld::UpdateChunksAtPosition(const FIntVector& Position, bool bAsync)
{
	Render->UpdateChunksAtPosition(Position, bAsync);
}

void AVoxelWorld::UpdateChunksOverlappingBox(const FVoxelBox& Box, bool bAsync)
{
	Render->UpdateChunksOverlappingBox(Box, bAsync);
}

void AVoxelWorld::UpdateAll(bool bAsync)
{
	Render->UpdateAll(bAsync);
}

void AVoxelWorld::AddInvoker(TWeakObjectPtr<UVoxelInvokerComponent> Invoker)
{
	Render->AddInvoker(Invoker);
}

void AVoxelWorld::CreateWorld()
{
	check(!IsCreated());

	UE_LOG(LogVoxel, Warning, TEXT("Loading world"));

	Depth = NewDepth;
	VoxelSize = NewVoxelSize;

	SetActorScale3D(FVector::OneVector);

	check(!Data);
	check(!Render);

	if (!InstancedWorldGenerator || InstancedWorldGenerator->GetClass() != WorldGenerator->GetClass())
	{
		InstancedWorldGenerator = nullptr;
		if (WorldGenerator)
		{
			// Create generator
			InstancedWorldGenerator = NewObject<UVoxelWorldGenerator>((UObject*)GetTransientPackage(), WorldGenerator);
		}
		if (InstancedWorldGenerator == nullptr)
		{
			UE_LOG(LogVoxel, Error, TEXT("Invalid world generator"));
			InstancedWorldGenerator = NewObject<UVoxelWorldGenerator>((UObject*)GetTransientPackage(), UFlatWorldGenerator::StaticClass());
		}
	}

	InstancedWorldGenerator->SetVoxelWorld(this);

	// Create Data
	Data = new FVoxelData(Depth, InstancedWorldGenerator);
#if DO_CHECK
	Data->TestWorldGenerator();
#endif

	// Create Render
	Render = new FVoxelRender(this, this, Data, MeshThreadCount, FoliageThreadCount);

	bIsCreated = true;
}

void AVoxelWorld::DestroyWorld()
{
	check(IsCreated());

	UE_LOG(LogVoxel, Warning, TEXT("Unloading world"));

	check(Render);
	check(Data);
	Render->Destroy();
	delete Render;
	delete Data; // Data must be deleted AFTER Render
	Render = nullptr;
	Data = nullptr;

	bIsCreated = false;
}

void AVoxelWorld::CreateInEditor()
{
	if (VoxelWorldEditorClass)
	{
		// Create/Find VoxelWorldEditor
		VoxelWorldEditor = nullptr;

		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), VoxelWorldEditorClass, FoundActors);

		for (auto Actor : FoundActors)
		{
			auto VoxelWorldEditorActor = Cast<AVoxelWorldEditorInterface>(Actor);
			if (VoxelWorldEditorActor)
			{
				VoxelWorldEditor = VoxelWorldEditorActor;
				break;
			}
		}
		if (!VoxelWorldEditor)
		{
			// else spawn
			VoxelWorldEditor = Cast<AVoxelWorldEditorInterface>(GetWorld()->SpawnActor(VoxelWorldEditorClass));
		}

		VoxelWorldEditor->Init(this);


		if (IsCreated())
		{
			DestroyWorld();
		}

		CreateWorld();

		bComputeCollisions = false;

		AddInvoker(VoxelWorldEditor->GetInvoker());

		UpdateAll(true);
	}
}

void AVoxelWorld::DestroyInEditor()
{
	if (IsCreated())
	{
		DestroyWorld();
	}
}

bool AVoxelWorld::IsCreated() const
{
	return bIsCreated;
}

int AVoxelWorld::GetDepthAt(const FIntVector& Position) const
{
	if (IsInWorld(Position))
	{
		return Render->GetDepthAt(Position);
	}
	else
	{
		UE_LOG(LogVoxel, Error, TEXT("GetDepthAt: Not in world: (%d, %d, %d)"), Position.X, Position.Y, Position.Z);
		return 0;
	}
}

bool AVoxelWorld::IsInWorld(const FIntVector& Position) const
{
	return Data->IsInWorld(Position.X, Position.Y, Position.Z);
}

bool AVoxelWorld::GetIntersection(const FIntVector& Start, const FIntVector& End, FVector& OutGlobalPosition, FIntVector& OutVoxelPosition)
{
	FIntVector Diff = End - Start;
	if (Diff.X != 0)
	{
		if (Diff.Y != 0 || Diff.Z != 0)
		{
			UE_LOG(LogVoxel, Error, TEXT("GetIntersection: Start and end should have 2 common coordinates"));
			return false;
		}
	}
	else if (Diff.Y != 0)
	{
		if (Diff.X != 0 || Diff.Z != 0)
		{
			UE_LOG(LogVoxel, Error, TEXT("GetIntersection: Start and end should have 2 common coordinates"));
			return false;
		}
	}
	else if (Diff.Z != 0)
	{
		if (Diff.X != 0 || Diff.Y != 0)
		{
			UE_LOG(LogVoxel, Error, TEXT("GetIntersection: Start and end should have 2 common coordinates"));
			return false;
		}
	}

	FIntVector RealStart(FMath::Min(Start.X, End.X), FMath::Min(Start.Y, End.Y), FMath::Min(Start.Z, End.Z));
	FIntVector RealEnd(FMath::Max(Start.X, End.X), FMath::Max(Start.Y, End.Y), FMath::Max(Start.Z, End.Z));

	bool bFound = false;
	Data->BeginGet();
	float OldValue = Data->GetValue(RealStart.X, RealStart.Y, RealStart.Z);
	FIntVector OldPosition = RealStart;
	for (int X = RealStart.X; X <= RealEnd.X; X++)
	{
		for (int Y = RealStart.Y; Y <= RealEnd.Y; Y++)
		{
			for (int Z = RealStart.Z; Z <= RealEnd.Z; Z++)
			{
				float Value = Data->GetValue(X, Y, Z);
				FIntVector Position(X, Y, Z);

				if ((OldValue > 0 && Value > 0) || (OldValue <= 0 && Value <= 0))
				{
					check(OldValue - Value != 0);
					const float t = OldValue / (OldValue - Value);

					FVector Q = t * static_cast<FVector>(Position) + (1 - t) * static_cast<FVector>(OldPosition);
					OutGlobalPosition = LocalToGlobal(Q);
					OutVoxelPosition = Position;
					bFound = true;
				}

				OldValue = Value;
				OldPosition = Position;

				if (bFound)
				{
					break;
				}
			}
			if (bFound)
			{
				break;
			}
		}
		if (bFound)
		{
			break;
		}
	}
	Data->EndGet();

	return bFound;
}

FVector AVoxelWorld::GetNormal(const FIntVector& Position) const
{
	int X = Position.X;
	int Y = Position.Y;
	int Z = Position.Z;

	Data->BeginGet();
	FVector Gradient;
	Gradient.X = Data->GetValue(X + 1, Y, Z) - Data->GetValue(X - 1, Y, Z);
	Gradient.Y = Data->GetValue(X, Y + 1, Z) - Data->GetValue(X, Y - 1, Z);
	Gradient.Z = Data->GetValue(X, Y, Z + 1) - Data->GetValue(X, Y, Z - 1);
	Data->EndGet();

	return Gradient.GetSafeNormal();
}

float AVoxelWorld::GetVoxelSize() const
{
	return VoxelSize;
}

int AVoxelWorld::Size() const
{
	return Data->Size();
}
