// Copyright 2017 Phyronnaz

#include "EmptyWorldGenerator.h"

void UEmptyWorldGenerator::GetValuesAndMaterials(float Values[], FVoxelMaterial Materials[], const FIntVector& Start, const FIntVector& StartIndex, const int Step, const FIntVector& Size, const FIntVector& ArraySize) const
{
	for (int K = 0; K < Size.Z; K++)
	{
		for (int J = 0; J < Size.Y; J++)
		{
			for (int I = 0; I < Size.X; I++)
			{
				const int Index = (StartIndex.X + I) + ArraySize.X * (StartIndex.Y + J) + ArraySize.X * ArraySize.Y * (StartIndex.Z + K);
				if (Values)
				{
					Values[Index] = 1;
				}
				if (Materials)
				{
					Materials[Index] = FVoxelMaterial();
				}
			}
		}
	}
}
