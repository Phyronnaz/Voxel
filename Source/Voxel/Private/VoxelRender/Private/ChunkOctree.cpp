// Copyright 2017 Phyronnaz

#include "ChunkOctree.h"
#include "VoxelChunkComponent.h"
#include "VoxelRender.h"
#include "VoxelInvokerComponent.h"

FChunkOctree::FChunkOctree(FVoxelRender* Render, FIntVector Position, uint8 Depth, uint64 Id)
	: FOctree(Position, Depth, Id)
	, Render(Render)
	, bHasChunk(false)
	, VoxelChunk(nullptr)
{
	check(Render);
};

void FChunkOctree::Delete()
{
	Render->RemoveChunkFromQueue(this);

	if (bHasChunk)
	{
		Unload();
	}
	if (bHasChilds)
	{
		DeleteChilds();
	}
}

void FChunkOctree::UpdateLOD(const std::deque<TWeakObjectPtr<UVoxelInvokerComponent>>& Invokers)
{
	check(bHasChunk == (VoxelChunk != nullptr));
	check(bHasChilds == (Childs.Num() == 8));

	if (Depth == 0)
	{
		// Always create
		if (!bHasChunk)
		{
			Load();
		}
		return;
	}

	const FVector ChunkWorldPosition = Render->GetGlobalPosition(Position);
	const float ChunkSide = Render->World->GetVoxelSize() * Size() / 2;

	float MinDistance = MAX_flt;
	for (auto Invoker : Invokers)
	{
		check(Invoker.IsValid());
		if (!Invoker->IsForPhysicsOnly())
		{
			const float Distance = FMath::Max(0.f, (ChunkWorldPosition - Invoker->GetOwner()->GetActorLocation()).GetAbsMax() - Invoker->DistanceOffset - ChunkSide);
			if (Distance < MinDistance)
			{
				MinDistance = Distance;
			}
		}
	}

	MinDistance /= Render->World->GetVoxelSize();

	MinDistance = FMath::Max(1.f, MinDistance);

	const float MinLOD = FMath::Log2(MinDistance / 16);

	// Tolerance zone to avoid reload loop when on a border
	const float MaxDistance = MinDistance + (16 << (int)MinLOD) / 2;
	const float MaxLOD = FMath::Log2(MaxDistance / 16);

	if (MinLOD < Depth && Depth < MaxLOD)
	{
		// Depth OK
		if (bHasChilds)
		{
			// Update childs
			for (int i = 0; i < 8; i++)
			{
				Childs[i]->UpdateLOD(Invokers);
			}
		}
		else if (!bHasChunk)
		{
			// Not created, create
			Load();
		}
	}
	else if (MaxLOD < Depth)
	{
		// Resolution too low
		if (bHasChunk)
		{
			Unload();
		}
		if (!bHasChilds)
		{
			CreateChilds();
		}
		if (bHasChilds)
		{
			// Update childs
			for (int i = 0; i < 8; i++)
			{
				Childs[i]->UpdateLOD(Invokers);
			}
		}
	}
	else // Depth < MinLOD
	{
		// Resolution too high
		if (bHasChilds)
		{
			// Too far, delete childs
			DeleteChilds();
		}
		if (!bHasChunk)
		{
			// Not created, create
			Load();
		}
	}
}

FChunkOctree* FChunkOctree::GetLeaf(const FIntVector& PointPosition)
{
	check(bHasChunk == (VoxelChunk != nullptr));
	check(bHasChilds == (Childs.Num() == 8));

	FChunkOctree* Current = this;

	while (!Current->bHasChunk)
	{
		Current = Current->GetChild(PointPosition);
	}
	return Current;
}

UVoxelChunkComponent* FChunkOctree::GetVoxelChunk() const
{
	return VoxelChunk;
}

FChunkOctree* FChunkOctree::GetChild(const FIntVector& PointPosition)
{
	check(bHasChilds);
	check(IsInOctree(PointPosition.X, PointPosition.Y, PointPosition.Z));

	// Ex: Child 6 -> position (0, 1, 1) -> 0b011 == 6
	FChunkOctree* Child = Childs[(PointPosition.X >= Position.X ? 1 : 0) + (PointPosition.Y >= Position.Y ? 2 : 0) + (PointPosition.Z >= Position.Z ? 4 : 0)];

	return Child;
}

void FChunkOctree::Load()
{
	check(!VoxelChunk);
	check(!bHasChunk);

	VoxelChunk = Render->GetInactiveChunk();
	VoxelChunk->Init(this);
	Render->UpdateChunk(this, true);
	bHasChunk = true;
}

void FChunkOctree::Unload()
{
	check(VoxelChunk);
	check(bHasChunk);

	VoxelChunk->Unload();

	VoxelChunk = nullptr;
	bHasChunk = false;
}

void FChunkOctree::CreateChilds()
{
	check(!bHasChilds);
	check(Depth != 0);

	int d = Size() / 4;
	uint64 Pow = IntPow9(Depth - 1);

	Childs.Add(new FChunkOctree(Render, Position + FIntVector(-d, -d, -d), Depth - 1, Id + 1 * Pow));
	Childs.Add(new FChunkOctree(Render, Position + FIntVector(+d, -d, -d), Depth - 1, Id + 2 * Pow));
	Childs.Add(new FChunkOctree(Render, Position + FIntVector(-d, +d, -d), Depth - 1, Id + 3 * Pow));
	Childs.Add(new FChunkOctree(Render, Position + FIntVector(+d, +d, -d), Depth - 1, Id + 4 * Pow));
	Childs.Add(new FChunkOctree(Render, Position + FIntVector(-d, -d, +d), Depth - 1, Id + 5 * Pow));
	Childs.Add(new FChunkOctree(Render, Position + FIntVector(+d, -d, +d), Depth - 1, Id + 6 * Pow));
	Childs.Add(new FChunkOctree(Render, Position + FIntVector(-d, +d, +d), Depth - 1, Id + 7 * Pow));
	Childs.Add(new FChunkOctree(Render, Position + FIntVector(+d, +d, +d), Depth - 1, Id + 8 * Pow));

	bHasChilds = true;
}

void FChunkOctree::DeleteChilds()
{
	check(bHasChilds);
	check(Childs.Num() == 8);

	for (FChunkOctree* Child : Childs)
	{
		Child->Delete();
		delete Child;
	}
	Childs.Reset();
	bHasChilds = false;
}

void FChunkOctree::GetLeafsOverlappingBox(const FVoxelBox& Box, std::deque<FChunkOctree*>& Octrees)
{
	FVoxelBox OctreeBox(GetMinimalCornerPosition(), GetMaximalCornerPosition());

	if (OctreeBox.Intersect(Box))
	{
		if (IsLeaf())
		{
			Octrees.push_front(this);
		}
		else
		{
			for (auto Child : Childs)
			{
				Child->GetLeafsOverlappingBox(Box, Octrees);
			}
		}
	}
}
