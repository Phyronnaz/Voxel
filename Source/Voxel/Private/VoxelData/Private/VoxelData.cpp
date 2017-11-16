// Copyright 2017 Phyronnaz

#include "VoxelData.h"
#include "VoxelPrivatePCH.h"
#include "ValueOctree.h"
#include "VoxelSave.h"
#include "GenericPlatformProcess.h"

FVoxelData::FVoxelData(int Depth, UVoxelWorldGenerator* WorldGenerator, bool bMultiplayer)
	: Depth(Depth)
	, WorldGenerator(WorldGenerator)
	, bMultiplayer(bMultiplayer)
{
	MainOctree = new FValueOctree(WorldGenerator, FIntVector::ZeroValue, Depth, FOctree::GetTopIdFromDepth(Depth), bMultiplayer);

	GetCount.Reset();

	CanGetEvent = FGenericPlatformProcess::GetSynchEventFromPool(true);
	CanSetEvent = FGenericPlatformProcess::GetSynchEventFromPool(true);

	CanGetEvent->Trigger();
	CanSetEvent->Trigger();
}

FVoxelData::~FVoxelData()
{
	delete MainOctree;
}

int FVoxelData::Size() const
{
	return 16 << Depth;
}

void FVoxelData::BeginSet()
{
	CanGetEvent->Reset();
	CanSetEvent->Wait();
	CanSetEvent->Reset();
}

void FVoxelData::EndSet()
{
	CanSetEvent->Trigger();
	CanGetEvent->Trigger();
}

void FVoxelData::BeginGet()
{
	CanGetEvent->Wait();
	CanSetEvent->Reset();
	GetCount.Increment();
}

void FVoxelData::EndGet()
{
	GetCount.Decrement();
	if (GetCount.GetValue() == 0)
	{
		CanSetEvent->Trigger();
	}
}

void FVoxelData::Reset()
{
	delete MainOctree;
	MainOctree = new FValueOctree(WorldGenerator, FIntVector::ZeroValue, Depth, FOctree::GetTopIdFromDepth(Depth), bMultiplayer);
}
void FVoxelData::GetValuesAndMaterials(float Values[], FVoxelMaterial Materials[], const FIntVector& Start, const FIntVector& StartIndex, const int Step, const FIntVector& Size, const FIntVector& ArraySize) const
{
	check(IsInWorld(Start.X, Start.Y, Start.Z));
	check(IsInWorld(Start.X + Size.X, Start.Y + Size.Y, Start.Z + Size.Z));
	MainOctree->GetValuesAndMaterials(Values, Materials, Start, StartIndex, Step, Size, ArraySize);
}

float FVoxelData::GetValue(int X, int Y, int Z)
{
	float Values[1];
	GetValuesAndMaterials(Values, nullptr, FIntVector(X, Y, Z), FIntVector::ZeroValue, 1, FIntVector(1, 1, 1), FIntVector(1, 1, 1));
	return Values[0];
}

FVoxelMaterial FVoxelData::GetMaterial(int X, int Y, int Z)
{
	FVoxelMaterial Materials[1];
	GetValuesAndMaterials(nullptr, Materials, FIntVector(X, Y, Z), FIntVector::ZeroValue, 1, FIntVector(1, 1, 1), FIntVector(1, 1, 1));
	return Materials[0];
}
void FVoxelData::SetValue(int X, int Y, int Z, float Value)
{
	check(IsInWorld(X, Y, Z));
	MainOctree->GetLeaf(X, Y, Z)->SetValueAndMaterial(X, Y, Z, Value, FVoxelMaterial(), true, false);
}

void FVoxelData::SetValue(int X, int Y, int Z, float Value, FValueOctree*& LastOctree)
{
	if (UNLIKELY(!LastOctree || !LastOctree->IsLeaf() || !LastOctree->IsInOctree(X, Y, Z)))
	{
		LastOctree = MainOctree->GetLeaf(X, Y, Z);
	}
	LastOctree->SetValueAndMaterial(X, Y, Z, Value, FVoxelMaterial(), true, false);
}

void FVoxelData::SetMaterial(int X, int Y, int Z, FVoxelMaterial Material)
{
	check(IsInWorld(X, Y, Z));
	MainOctree->GetLeaf(X, Y, Z)->SetValueAndMaterial(X, Y, Z, 0, Material, false, true);
}

void FVoxelData::SetMaterial(int X, int Y, int Z, FVoxelMaterial Material, FValueOctree*& LastOctree)
{
	if (UNLIKELY(!LastOctree || !LastOctree->IsLeaf() || !LastOctree->IsInOctree(X, Y, Z)))
	{
		LastOctree = MainOctree->GetLeaf(X, Y, Z);
	}
	LastOctree->SetValueAndMaterial(X, Y, Z, 0, Material, false, true);
}

void FVoxelData::SetValueAndMaterial(int X, int Y, int Z, float Value, FVoxelMaterial Material, FValueOctree*& LastOctree)
{
	if (UNLIKELY(!LastOctree || !LastOctree->IsLeaf() || !LastOctree->IsInOctree(X, Y, Z)))
	{
		LastOctree = MainOctree->GetLeaf(X, Y, Z);
	}
	LastOctree->SetValueAndMaterial(X, Y, Z, Value, Material, true, true);
}

bool FVoxelData::IsInWorld(int X, int Y, int Z) const
{
	int S = Size() / 2;
	return -S <= X && X < S
		&& -S <= Y && Y < S
		&& -S <= Z && Z < S;
}

FORCEINLINE void FVoxelData::ClampToWorld(int& X, int& Y, int& Z) const
{
	int S = Size() / 2;
	X = FMath::Clamp(X, -S, S - 1);
	Y = FMath::Clamp(Y, -S, S - 1);
	Z = FMath::Clamp(Z, -S, S - 1);
}

void FVoxelData::GetSave(FVoxelWorldSave& OutSave) const
{
	std::list<TSharedRef<FVoxelChunkSave>> SaveList;
	MainOctree->AddDirtyChunksToSaveList(SaveList);
	OutSave.Init(Depth, SaveList);
}

void FVoxelData::LoadFromSaveAndGetModifiedPositions(FVoxelWorldSave& Save, std::forward_list<FIntVector>& OutModifiedPositions, bool bReset)
{
	BeginSet();
	if (bReset)
	{
		MainOctree->GetDirtyChunksPositions(OutModifiedPositions);
		Reset();
	}

	auto SaveList = Save.GetChunksList();
	MainOctree->LoadFromSaveAndGetModifiedPositions(SaveList, OutModifiedPositions);
	check(SaveList.empty());
	EndSet();
}

void FVoxelData::GetDiffLists(std::forward_list<FVoxelValueDiff>& OutValueDiffList, std::forward_list<FVoxelMaterialDiff>& OutMaterialDiffList) const
{
	MainOctree->AddChunksToDiffLists(OutValueDiffList, OutMaterialDiffList);
}

void FVoxelData::LoadFromDiffListsAndGetModifiedPositions(std::forward_list<FVoxelValueDiff> ValueDiffList, std::forward_list<FVoxelMaterialDiff> MaterialDiffList, std::forward_list<FIntVector>& OutModifiedPositions)
{
	BeginSet();
	MainOctree->LoadFromDiffListsAndGetModifiedPositions(ValueDiffList, MaterialDiffList, OutModifiedPositions);
	EndSet();
}
