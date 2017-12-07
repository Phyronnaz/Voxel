// Copyright 2017 Phyronnaz

#include "ValueOctree.h"
#include "VoxelWorldGenerator.h"

FValueOctree::FValueOctree(UVoxelWorldGenerator* WorldGenerator, FIntVector Position, uint8 Depth, uint64 Id)
	: FOctree(Position, Depth, Id)
	, WorldGenerator(WorldGenerator)
	, bIsDirty(false)
{

}

FValueOctree::~FValueOctree()
{
	if (bHasChilds)
	{
		for (auto Child : Childs)
		{
			delete Child;
		}
	}
}

bool FValueOctree::IsDirty() const
{
	return bIsDirty;
}

void FValueOctree::GetValuesAndMaterials(float InValues[], FVoxelMaterial InMaterials[], const FIntVector& Start, const FIntVector& StartIndex, const int Step, const FIntVector& Size, const FIntVector& ArraySize) const
{
	check(Size.GetMin() >= 0);
	if (Size.X == 0 || Size.Y == 0 || Size.Z == 0)
	{
		return;
	}
	check(IsInOctree(Start.X, Start.Y, Start.Z));
	check(IsInOctree(Start.X + (Size.X - 1) * Step, Start.Y + (Size.Y - 1) * Step, Start.Z + (Size.Z - 1) * Step));

	if (IsLeaf())
	{
		if (IsDirty())
		{
			for (int I = 0; I < Size.X; I++)
			{
				for (int J = 0; J < Size.Y; J++)
				{
					for (int K = 0; K < Size.Z; K++)
					{
						const int X = Start.X + I * Step;
						const int Y = Start.Y + J * Step;
						const int Z = Start.Z + K * Step;

						int LocalX, LocalY, LocalZ;
						GlobalToLocal(X, Y, Z, LocalX, LocalY, LocalZ);

						const int LocalIndex = IndexFromCoordinates(LocalX, LocalY, LocalZ);
						const int Index = (StartIndex.X + I) + ArraySize.X * (StartIndex.Y + J) + ArraySize.X * ArraySize.Y * (StartIndex.Z + K);

						if (InValues)
						{
							InValues[Index] = Values[LocalIndex];
						}
						if (InMaterials)
						{
							InMaterials[Index] = Materials[LocalIndex];
						}
					}
				}
			}
		}
		else
		{
			WorldGenerator->GetValuesAndMaterials(InValues, InMaterials, Start, StartIndex, Step, Size, ArraySize);
		}
	}
	else if (Size.X == 1 && Size.Y == 1 && Size.Z == 1 && false)
	{
		GetChild(Start.X, Start.Y, Start.Z)->GetValuesAndMaterials(InValues, InMaterials, Start, StartIndex, Step, Size, ArraySize);
	}
	else
	{
		const int StartI = StartIndex.X;
		const int StartJ = StartIndex.Y;
		const int StartK = StartIndex.Z;

		const int StartX = Start.X;
		const int StartY = Start.Y;
		const int StartZ = Start.Z;

		const int RealSizeX = (Size.X - 1) * Step;
		const int RealSizeY = (Size.Y - 1) * Step;
		const int RealSizeZ = (Size.Z - 1) * Step;



		const int StartXBot = StartX;
		const int StartIBot = StartI;
		const int SizeXBot = (Position.X - StartX) / Step;
		const int StartXTop = StartXBot + SizeXBot * Step;
		const int StartITop = StartIBot + SizeXBot;
		const int SizeXTop = Size.X - SizeXBot;

		const int StartYBot = StartY;
		const int StartJBot = StartJ;
		const int SizeYBot = (Position.Y - StartY) / Step;
		const int StartYTop = StartYBot + SizeYBot * Step;
		const int StartJTop = StartJBot + SizeYBot;
		const int SizeYTop = Size.Y - SizeYBot;

		const int StartZBot = StartZ;
		const int StartKBot = StartK;
		const int SizeZBot = (Position.Z - StartZ) / Step;
		const int StartZTop = StartZBot + SizeZBot * Step;
		const int StartKTop = StartKBot + SizeZBot;
		const int SizeZTop = Size.Z - SizeZBot;


		if (StartX + RealSizeX < Position.X || Position.X <= StartX)
		{
			if (StartY + RealSizeY < Position.Y || Position.Y <= StartY)
			{
				if (StartZ + RealSizeZ < Position.Z || Position.Z <= StartZ)
				{
					GetChild(StartX, StartY, StartZ)->GetValuesAndMaterials(InValues, InMaterials,
						{ StartX, StartY, StartZ },
						{ StartI, StartJ, StartK },
						Step,
						{ Size.X, Size.Y, Size.Z },
						ArraySize);
				}
				else
				{
					// Split Z

					GetChild(StartX, StartY, StartZ)->GetValuesAndMaterials(InValues, InMaterials,
						{ StartX, StartY, StartZBot },
						{ StartI, StartJ, StartKBot },
						Step,
						{ Size.X, Size.Y, SizeZBot },
						ArraySize);

					GetChild(StartX, StartY, StartZ + RealSizeZ)->GetValuesAndMaterials(InValues, InMaterials,
						{ StartX, StartY, StartZTop },
						{ StartI, StartJ, StartKTop },
						Step,
						{ Size.X, Size.Y, SizeZTop },
						ArraySize);
				}
			}
			else
			{
				// Split Y

				if (StartZ + RealSizeZ < Position.Z || Position.Z <= StartZ)
				{
					GetChild(StartX, StartY, StartZ)->GetValuesAndMaterials(InValues, InMaterials,
						{ StartX, StartYBot, StartZ },
						{ StartI, StartJBot, StartK },
						Step,
						{ Size.X, SizeYBot, Size.Z },
						ArraySize);

					GetChild(StartX, StartY + RealSizeY, StartZ)->GetValuesAndMaterials(InValues, InMaterials,
						{ StartX, StartYTop, StartZ },
						{ StartI, StartJTop, StartK },
						Step,
						{ Size.X, SizeYTop, Size.Z },
						ArraySize);
				}
				else
				{
					// Split Z

					GetChild(StartX, StartY, StartZ)->GetValuesAndMaterials(InValues, InMaterials,
						{ StartX, StartYBot, StartZBot },
						{ StartI, StartJBot, StartKBot },
						Step,
						{ Size.X, SizeYBot, SizeZBot },
						ArraySize);

					GetChild(StartX, StartY, StartZ + RealSizeZ)->GetValuesAndMaterials(InValues, InMaterials,
						{ StartX, StartYBot, StartZTop },
						{ StartI, StartJBot, StartKTop },
						Step,
						{ Size.X, SizeYBot, SizeZTop },
						ArraySize);


					GetChild(StartX, StartY + RealSizeY, StartZ)->GetValuesAndMaterials(InValues, InMaterials,
						{ StartX, StartYTop, StartZBot },
						{ StartI, StartJTop, StartKBot },
						Step,
						{ Size.X, SizeYTop, SizeZBot },
						ArraySize);

					GetChild(StartX, StartY + RealSizeY, StartZ + RealSizeZ)->GetValuesAndMaterials(InValues, InMaterials,
						{ StartX, StartYTop, StartZTop },
						{ StartI, StartJTop, StartKTop },
						Step,
						{ Size.X, SizeYTop, SizeZTop },
						ArraySize);
				}
			}
		}
		else
		{
			// Split X

			if (StartY + RealSizeY < Position.Y || Position.Y <= StartY)
			{
				if (StartZ + RealSizeZ < Position.Z || Position.Z <= StartZ)
				{
					GetChild(StartX, StartY, StartZ)->GetValuesAndMaterials(InValues, InMaterials,
						{ StartXBot, StartY, StartZ },
						{ StartIBot, StartJ, StartK },
						Step,
						{ SizeXBot, Size.Y, Size.Z },
						ArraySize);

					GetChild(StartX + RealSizeX, StartY, StartZ)->GetValuesAndMaterials(InValues, InMaterials,
						{ StartXTop, StartY, StartZ },
						{ StartITop, StartJ, StartK },
						Step,
						{ SizeXTop, Size.Y, Size.Z },
						ArraySize);
				}
				else
				{
					// Split Z

					GetChild(StartX, StartY, StartZ)->GetValuesAndMaterials(InValues, InMaterials,
						{ StartXBot, StartY, StartZBot },
						{ StartIBot, StartJ, StartKBot },
						Step,
						{ SizeXBot, Size.Y, SizeZBot },
						ArraySize);

					GetChild(StartX, StartY, StartZ + RealSizeZ)->GetValuesAndMaterials(InValues, InMaterials,
						{ StartXBot, StartY, StartZTop },
						{ StartIBot, StartJ, StartKTop },
						Step,
						{ SizeXBot, Size.Y, SizeZTop },
						ArraySize);


					GetChild(StartX + RealSizeX, StartY, StartZ)->GetValuesAndMaterials(InValues, InMaterials,
						{ StartXTop, StartY, StartZBot },
						{ StartITop, StartJ, StartKBot },
						Step,
						{ SizeXTop, Size.Y, SizeZBot },
						ArraySize);

					GetChild(StartX + RealSizeX, StartY, StartZ + RealSizeZ)->GetValuesAndMaterials(InValues, InMaterials,
						{ StartXTop, StartY, StartZTop },
						{ StartITop, StartJ, StartKTop },
						Step,
						{ SizeXTop, Size.Y, SizeZTop },
						ArraySize);
				}
			}
			else
			{
				// Split Y

				if (StartZ + RealSizeZ < Position.Z || Position.Z <= StartZ)
				{
					GetChild(StartX, StartY, StartZ)->GetValuesAndMaterials(InValues, InMaterials,
						{ StartXBot, StartYBot, StartZ },
						{ StartIBot, StartJBot, StartK },
						Step,
						{ SizeXBot, SizeYBot, Size.Z },
						ArraySize);

					GetChild(StartX, StartY + RealSizeY, StartZ)->GetValuesAndMaterials(InValues, InMaterials,
						{ StartXBot, StartYTop, StartZ },
						{ StartIBot, StartJTop, StartK },
						Step,
						{ SizeXBot, SizeYTop, Size.Z },
						ArraySize);

					GetChild(StartX + RealSizeX, StartY, StartZ)->GetValuesAndMaterials(InValues, InMaterials,
						{ StartXTop, StartYBot, StartZ },
						{ StartITop, StartJBot, StartK },
						Step,
						{ SizeXTop, SizeYBot, Size.Z },
						ArraySize);

					GetChild(StartX + RealSizeX, StartY + RealSizeY, StartZ)->GetValuesAndMaterials(InValues, InMaterials,
						{ StartXTop, StartYTop, StartZ },
						{ StartITop, StartJTop, StartK },
						Step,
						{ SizeXTop, SizeYTop, Size.Z },
						ArraySize);
				}
				else
				{
					// Split Z

					GetChild(StartX, StartY, StartZ)->GetValuesAndMaterials(InValues, InMaterials,
						{ StartXBot, StartYBot, StartZBot },
						{ StartIBot, StartJBot, StartKBot },
						Step,
						{ SizeXBot, SizeYBot, SizeZBot },
						ArraySize);

					GetChild(StartX, StartY, StartZ + RealSizeZ)->GetValuesAndMaterials(InValues, InMaterials,
						{ StartXBot, StartYBot, StartZTop },
						{ StartIBot, StartJBot, StartKTop },
						Step,
						{ SizeXBot, SizeYBot, SizeZTop },
						ArraySize);


					GetChild(StartX, StartY + RealSizeY, StartZ)->GetValuesAndMaterials(InValues, InMaterials,
						{ StartXBot, StartYTop, StartZBot },
						{ StartIBot, StartJTop, StartKBot },
						Step,
						{ SizeXBot, SizeYTop, SizeZBot },
						ArraySize);

					GetChild(StartX, StartY + RealSizeY, StartZ + RealSizeZ)->GetValuesAndMaterials(InValues, InMaterials,
						{ StartXBot, StartYTop, StartZTop },
						{ StartIBot, StartJTop, StartKTop },
						Step,
						{ SizeXBot, SizeYTop, SizeZTop },
						ArraySize);





					GetChild(StartX + RealSizeX, StartY, StartZ)->GetValuesAndMaterials(InValues, InMaterials,
						{ StartXTop, StartYBot, StartZBot },
						{ StartITop, StartJBot, StartKBot },
						Step,
						{ SizeXTop, SizeYBot, SizeZBot },
						ArraySize);

					GetChild(StartX + RealSizeX, StartY, StartZ + RealSizeZ)->GetValuesAndMaterials(InValues, InMaterials,
						{ StartXTop, StartYBot, StartZTop },
						{ StartITop, StartJBot, StartKTop },
						Step,
						{ SizeXTop, SizeYBot, SizeZTop },
						ArraySize);


					GetChild(StartX + RealSizeX, StartY + RealSizeY, StartZ)->GetValuesAndMaterials(InValues, InMaterials,
						{ StartXTop, StartYTop, StartZBot },
						{ StartITop, StartJTop, StartKBot },
						Step,
						{ SizeXTop, SizeYTop, SizeZBot },
						ArraySize);

					GetChild(StartX + RealSizeX, StartY + RealSizeY, StartZ + RealSizeZ)->GetValuesAndMaterials(InValues, InMaterials,
						{ StartXTop, StartYTop, StartZTop },
						{ StartITop, StartJTop, StartKTop },
						Step,
						{ SizeXTop, SizeYTop, SizeZTop },
						ArraySize);
				}
			}
		}
	}
}

void FValueOctree::SetValueAndMaterial(int X, int Y, int Z, float Value, FVoxelMaterial Material, bool bSetValue, bool bSetMaterial)
{
	check(IsLeaf());
	check(IsInOctree(X, Y, Z));

	if (Depth != 0)
	{
		CreateChilds();
		bIsDirty = true;
		GetChild(X, Y, Z)->SetValueAndMaterial(X, Y, Z, Value, Material, bSetValue, bSetMaterial);
	}
	else
	{
		if (!IsDirty())
		{
			SetAsDirty();
		}

		int LocalX, LocalY, LocalZ;
		GlobalToLocal(X, Y, Z, LocalX, LocalY, LocalZ);

		int Index = IndexFromCoordinates(LocalX, LocalY, LocalZ);
		if (bSetValue)
		{
			Values[Index] = Value;
		}
		if (bSetMaterial)
		{
			Materials[Index] = Material;
		}
	}
}

void FValueOctree::AddDirtyChunksToSaveList(std::deque<TSharedRef<FVoxelChunkSave>>& SaveList)
{
	check(!IsLeaf() == (Childs.Num() == 8));
	check(!(IsDirty() && IsLeaf() && Depth != 0));

	if (IsDirty())
	{
		if (IsLeaf())
		{
			auto SaveStruct = TSharedRef<FVoxelChunkSave>(new FVoxelChunkSave(Id, Position, Values, Materials));
			SaveList.push_back(SaveStruct);
		}
		else
		{
			for (auto Child : Childs)
			{
				Child->AddDirtyChunksToSaveList(SaveList);
			}
		}
	}
}

void FValueOctree::LoadFromSaveAndGetModifiedPositions(std::deque<FVoxelChunkSave>& Save, std::deque<FIntVector>& OutModifiedPositions)
{
	if (Save.empty())
	{
		return;
	}

	if (Depth == 0)
	{
		if (Save.front().Id == Id)
		{
			bIsDirty = true;
			for (int X = 0; X < 16; X++)
			{
				for (int Y = 0; Y < 16; Y++)
				{
					for (int Z = 0; Z < 16; Z++)
					{
						const int Index = X + 16 * Y + 16 * 16 * Z;
						Values[Index] = Save.front().Values[Index];
						Materials[Index] = Save.front().Materials[Index];
					}
				}
			}
			Save.pop_front();

			// Update neighbors
			const int S = Size();
			OutModifiedPositions.push_front(Position - FIntVector(0, 0, 0));
			OutModifiedPositions.push_front(Position - FIntVector(S, 0, 0));
			OutModifiedPositions.push_front(Position - FIntVector(0, S, 0));
			OutModifiedPositions.push_front(Position - FIntVector(S, S, 0));
			OutModifiedPositions.push_front(Position - FIntVector(0, 0, S));
			OutModifiedPositions.push_front(Position - FIntVector(S, 0, S));
			OutModifiedPositions.push_front(Position - FIntVector(0, S, S));
			OutModifiedPositions.push_front(Position - FIntVector(S, S, S));
		}
	}
	else
	{
		uint64 Pow = IntPow9(Depth);
		if (Id / Pow == Save.front().Id / Pow)
		{
			if (IsLeaf())
			{
				CreateChilds();
				bIsDirty = true;
			}
			for (auto Child : Childs)
			{
				Child->LoadFromSaveAndGetModifiedPositions(Save, OutModifiedPositions);
			}
		}
	}
}

void FValueOctree::CreateChilds()
{
	check(IsLeaf());
	check(Childs.Num() == 0);
	check(Depth != 0);

	int d = Size() / 4;
	uint64 Pow = IntPow9(Depth - 1);

	Childs.Add(new FValueOctree(WorldGenerator, Position + FIntVector(-d, -d, -d), Depth - 1, Id + 1 * Pow));
	Childs.Add(new FValueOctree(WorldGenerator, Position + FIntVector(+d, -d, -d), Depth - 1, Id + 2 * Pow));
	Childs.Add(new FValueOctree(WorldGenerator, Position + FIntVector(-d, +d, -d), Depth - 1, Id + 3 * Pow));
	Childs.Add(new FValueOctree(WorldGenerator, Position + FIntVector(+d, +d, -d), Depth - 1, Id + 4 * Pow));
	Childs.Add(new FValueOctree(WorldGenerator, Position + FIntVector(-d, -d, +d), Depth - 1, Id + 5 * Pow));
	Childs.Add(new FValueOctree(WorldGenerator, Position + FIntVector(+d, -d, +d), Depth - 1, Id + 6 * Pow));
	Childs.Add(new FValueOctree(WorldGenerator, Position + FIntVector(-d, +d, +d), Depth - 1, Id + 7 * Pow));
	Childs.Add(new FValueOctree(WorldGenerator, Position + FIntVector(+d, +d, +d), Depth - 1, Id + 8 * Pow));

	bHasChilds = true;
	check(!IsLeaf() == (Childs.Num() == 8));
}

void FValueOctree::SetAsDirty()
{
	check(!IsDirty());
	check(Depth == 0);

	FIntVector Min = GetMinimalCornerPosition();
	GetValuesAndMaterials(Values, Materials, FIntVector(Min.X, Min.Y, Min.Z), FIntVector::ZeroValue, 1, FIntVector(16, 16, 16), FIntVector(16, 16, 16));

	bIsDirty = true;
}

int FValueOctree::IndexFromCoordinates(int X, int Y, int Z) const
{
	check(0 <= X && X < 16);
	check(0 <= Y && Y < 16);
	check(0 <= Z && Z < 16);
	return X + 16 * Y + 16 * 16 * Z;
}

void FValueOctree::CoordinatesFromIndex(int Index, int& OutX, int& OutY, int& OutZ) const
{
	check(0 <= Index && Index < 16 * 16 * 16);

	OutX = Index % 16;

	Index = (Index - OutX) / 16;
	OutY = Index % 16;

	Index = (Index - OutY) / 16;
	OutZ = Index;
}

FValueOctree * FValueOctree::GetChild(int X, int Y, int Z) const
{
	check(!IsLeaf());
	// Ex: Child 6 -> position (0, 1, 1) -> 0b011 == 6
	return Childs[(X >= Position.X) + 2 * (Y >= Position.Y) + 4 * (Z >= Position.Z)];
}

FValueOctree* FValueOctree::GetLeaf(int X, int Y, int Z)
{
	check(IsInOctree(X, Y, Z));

	FValueOctree* Ptr = this;

	while (!Ptr->IsLeaf())
	{
		Ptr = Ptr->GetChild(X, Y, Z);
	}

	check(Ptr->IsInOctree(X, Y, Z));

	return Ptr;
}

void FValueOctree::GetDirtyChunksPositions(std::deque<FIntVector>& OutPositions)
{
	if (IsDirty())
	{
		if (IsLeaf())
		{
			// With neighbors
			const int S = Size();
			OutPositions.push_front(Position - FIntVector(0, 0, 0));
			OutPositions.push_front(Position - FIntVector(S, 0, 0));
			OutPositions.push_front(Position - FIntVector(0, S, 0));
			OutPositions.push_front(Position - FIntVector(S, S, 0));
			OutPositions.push_front(Position - FIntVector(0, 0, S));
			OutPositions.push_front(Position - FIntVector(S, 0, S));
			OutPositions.push_front(Position - FIntVector(0, S, S));
			OutPositions.push_front(Position - FIntVector(S, S, S));
		}
		else
		{
			for (auto Child : Childs)
			{
				Child->GetDirtyChunksPositions(OutPositions);
			}
		}
	}
}