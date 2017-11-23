// Copyright 2017 Phyronnaz

#include "CollisionMeshHandler.h"
#include "VoxelData.h"
#include "VoxelInvokerComponent.h"
#include "VoxelWorld.h"
#include "VoxelPolygonizerForCollisions.h"


DECLARE_CYCLE_STAT(TEXT("CollisionMeshHandler ~ UpdateCollision"), STAT_UpdateCollision, STATGROUP_Voxel);

FAsyncCollisionTask::FAsyncCollisionTask(FVoxelData* Data, FIntVector ChunkPosition, bool bEnableRender)
	: Data(Data)
	, ChunkPosition(ChunkPosition)
	, bEnableRender(bEnableRender)
{

}

void FAsyncCollisionTask::DoWork()
{
	FVoxelPolygonizerForCollisions* Poly = new FVoxelPolygonizerForCollisions(Data, ChunkPosition, bEnableRender);
	Poly->CreateSection(Section);
	delete Poly;
}




FCollisionMeshHandler::FCollisionMeshHandler(TWeakObjectPtr<UVoxelInvokerComponent> const Invoker, AVoxelWorld* const World, FQueuedThreadPool* const Pool)
	: Invoker(Invoker)
	, World(World)
	, Pool(Pool)
{
	check(Invoker.IsValid());
	CurrentCenter = World->GlobalToLocal(Invoker.Get()->GetOwner()->GetActorLocation());

	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < 2; j++)
		{
			for (int k = 0; k < 2; k++)
			{
				UVoxelProceduralMeshComponent* Chunk = NewObject<UVoxelProceduralMeshComponent>(World, NAME_None, RF_Transient | RF_NonPIEDuplicateTransient);
				Chunk->bUseAsyncCooking = true;
				Chunk->SetupAttachment(World->GetRootComponent(), NAME_None);
				Chunk->RegisterComponent();
				Chunk->SetWorldScale3D(FVector::OneVector * World->GetVoxelSize());
				Chunk->SetWorldLocation(World->LocalToGlobal(CurrentCenter + FIntVector(i - 1, j - 1, k - 1) * CHUNKSIZE_FC));
				Components[i][j][k] = Chunk;
				Tasks[i][j][k] = nullptr;
				Update(i, j, k);
			}
		}
	}
	EndTasksTick();
}

FCollisionMeshHandler::~FCollisionMeshHandler()
{
	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < 2; j++)
		{
			for (int k = 0; k < 2; k++)
			{
				if (Tasks[i][j][k])
				{
					Tasks[i][j][k]->EnsureCompletion();
					delete Tasks[i][j][k];
				}
			}
		}
	}
}

void FCollisionMeshHandler::StartTasksTick()
{
	if (!Invoker.IsValid())
	{
		UE_LOG(LogVoxel, Error, TEXT("Invalid invoker"));
		return;
	}
	const FIntVector NewPosition = World->GlobalToLocal(Invoker.Get()->GetOwner()->GetActorLocation());
	const FIntVector Delta = NewPosition - CurrentCenter;

	if (Delta.X > CHUNKSIZE_FC / 2)
	{
		UVoxelProceduralMeshComponent* tmp[2][2];

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
		UVoxelProceduralMeshComponent* tmp[2][2];

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
		UVoxelProceduralMeshComponent* tmp[2][2];

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
		UVoxelProceduralMeshComponent* tmp[2][2];

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
		UVoxelProceduralMeshComponent* tmp[2][2];

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
		UVoxelProceduralMeshComponent* tmp[2][2];

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

void FCollisionMeshHandler::EndTasksTick()
{
	SCOPE_CYCLE_COUNTER(STAT_UpdateCollision);

	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < 2; j++)
		{
			for (int k = 0; k < 2; k++)
			{
				auto& Task = Tasks[i][j][k];
				auto& Component = Components[i][j][k];
				if (Task)
				{
					Task->EnsureCompletion();
					Component->SetProcMeshSection(0, Task->GetTask().Section);
					Component->SetWorldLocation(World->LocalToGlobal(Task->GetTask().ChunkPosition), false, nullptr, ETeleportType::None);
					delete Task;
					Task = nullptr;
				}
			}
		}
	}
}
bool FCollisionMeshHandler::IsValid()
{
	return Invoker.IsValid();
};

void  FCollisionMeshHandler::Destroy()
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

void  FCollisionMeshHandler::UpdateInBox(FVoxelBox Box)
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

void  FCollisionMeshHandler::Update(bool bXMax, bool bYMax, bool bZMax)
{
	const FIntVector ChunkPosition = CurrentCenter + FIntVector(bXMax - 1, bYMax - 1, bZMax - 1) * CHUNKSIZE_FC;

	auto& Task = Tasks[bXMax][bYMax][bZMax];
	auto& Component = Components[bXMax][bYMax][bZMax];

	check(!Task);
	Task = new FAsyncTask<FAsyncCollisionTask>(World->GetData(), ChunkPosition, World->GetDebugCollisions());
	Task->StartBackgroundTask(Pool);
}