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

void FVoxelData::TestWorldGenerator(FIntVector Position, FIntVector InSize)
{
	BeginGet();
	float* CachedValues = new float[InSize.X * InSize.Y * InSize.Z];
	FVoxelMaterial* CachedMaterials = new FVoxelMaterial[InSize.X * InSize.Y * InSize.Z];
	for (int Step = 1; Step < 3; Step++)
	{
		GetValuesAndMaterials(CachedValues, CachedMaterials, Position, FIntVector::ZeroValue, Step, InSize, InSize);

		for (int X = 0; X < InSize.X; X++)
		{
			for (int Y = 0; Y < InSize.Y; Y++)
			{
				for (int Z = 0; Z < InSize.Z; Z++)
				{
					float Value;
					FVoxelMaterial Material;
					GetValueAndMaterial(Position.X + X * Step, Position.Y + Y * Step, Position.Z + Z * Step, Value, Material);

					const int Index = X + InSize.X * Y + InSize.X * InSize.Y * Z;
					float CachedValue = CachedValues[Index];
					FVoxelMaterial CachedMaterial = CachedMaterials[Index];

					checkf(Value == CachedValue, TEXT("Invalid world generator! Values returned are not coherent for different Step"));
					checkf(Material == CachedMaterial, TEXT("Invalid world generator! Materials returned are not coherent for different Step"));
				}
			}
		}
	}
	delete CachedValues;
	delete CachedMaterials;
	EndGet();
}

void FVoxelData::GetValuesAndMaterials(float Values[], FVoxelMaterial Materials[], const FIntVector& Start, const FIntVector& StartIndex, const int Step, const FIntVector& InSize, const FIntVector& ArraySize) const
{
	if (InSize.X <= 0 || InSize.Y <= 0 || InSize.Z <= 0)
	{
		return;
	}
	if (UNLIKELY(!IsInWorld(Start.X, Start.Y, Start.Z) || !IsInWorld(Start.X + (InSize.X - 1) * Step, Start.Y + (InSize.Y - 1) * Step, Start.Z + (InSize.Z - 1) * Step)))
	{
		for (int X = 0; X < InSize.X; X++)
		{
			for (int Y = 0; Y < InSize.Y; Y++)
			{
				for (int Z = 0; Z < InSize.Z; Z++)
				{
					const int Index = (StartIndex.X + X) + ArraySize.X * (StartIndex.Y + Y) + ArraySize.X * ArraySize.Y * (StartIndex.Z + Z);
					int RX = Start.X + X * Step;
					int RY = Start.Y + Y * Step;
					int RZ = Start.Z + Z * Step;
					if (Values)
					{
						if (IsInWorld(RX, RY, RZ))
						{
							Values[Index] = GetValue(RX, RY, RZ);
						}
						else
						{
							float CValues[1];
							WorldGenerator->GetValuesAndMaterials(CValues, nullptr, FIntVector(RX, RY, RZ), FIntVector::ZeroValue, 1, FIntVector(1, 1, 1), FIntVector(1, 1, 1));
							Values[Index] = CValues[0];
						}
					}
					if (Materials)
					{
						if (IsInWorld(RX, RY, RZ))
						{
							Materials[Index] = GetMaterial(RX, RY, RZ);
						}
						else
						{
							FVoxelMaterial CMaterials[1];
							WorldGenerator->GetValuesAndMaterials(nullptr, CMaterials, FIntVector(RX, RY, RZ), FIntVector::ZeroValue, 1, FIntVector(1, 1, 1), FIntVector(1, 1, 1));
							Materials[Index] = CMaterials[0];
						}
					}
				}
			}
		}
	}
	else
	{
		MainOctree->GetValuesAndMaterials(Values, Materials, Start, StartIndex, Step, InSize, ArraySize);
	}
}

float FVoxelData::GetValue(int X, int Y, int Z) const
{
	float Values[1];
	GetValuesAndMaterials(Values, nullptr, FIntVector(X, Y, Z), FIntVector::ZeroValue, 1, FIntVector(1, 1, 1), FIntVector(1, 1, 1));
	return Values[0];
}

FVoxelMaterial FVoxelData::GetMaterial(int X, int Y, int Z) const
{
	FVoxelMaterial Materials[1];
	GetValuesAndMaterials(nullptr, Materials, FIntVector(X, Y, Z), FIntVector::ZeroValue, 1, FIntVector(1, 1, 1), FIntVector(1, 1, 1));
	return Materials[0];
}

void FVoxelData::GetValueAndMaterial(int X, int Y, int Z, float& OutValue, FVoxelMaterial& OutMaterial)
{
	float Values[1];
	FVoxelMaterial Materials[1];
	GetValuesAndMaterials(Values, Materials, FIntVector(X, Y, Z), FIntVector::ZeroValue, 1, FIntVector(1, 1, 1), FIntVector(1, 1, 1));

	OutValue = Values[0];
	OutMaterial = Materials[0];
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
