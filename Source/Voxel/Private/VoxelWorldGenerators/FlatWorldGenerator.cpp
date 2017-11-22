// Copyright 2017 Phyronnaz

#include "FlatWorldGenerator.h"

UFlatWorldGenerator::UFlatWorldGenerator()
	: TerrainHeight(0)
	, HardnessMultiplier(1)
	, FadeHeight(4)
{

}

void UFlatWorldGenerator::GetValuesAndMaterials(float Values[], FVoxelMaterial Materials[], const FIntVector& Start, const FIntVector& StartIndex, const int Step, const FIntVector& Size, const FIntVector& ArraySize) const
{
	for (int K = 0; K < Size.Z; K++)
	{
		const int Z = Start.Z + K * Step;
		const float Value = (Z >= TerrainHeight) ? HardnessMultiplier : -HardnessMultiplier;
		FVoxelMaterial Material = FVoxelMaterial();
		if (Materials && TerrainLayers.Num())
		{
			if (Z < TerrainLayers[0].Start)
			{
				Material = FVoxelMaterial(TerrainLayers[0].Material, TerrainLayers[0].Material, 255);
			}
			else
			{
				const int LastIndex = TerrainLayers.Num() - 1;
				if (LastIndex >= 0 && Z >= TerrainLayers[LastIndex].Start)
				{
					Material = FVoxelMaterial(TerrainLayers[LastIndex].Material, TerrainLayers[LastIndex].Material, (LastIndex % 2 == 0) ? 255 : 0);
				}
				else
				{
					for (int i = 0; i < TerrainLayers.Num() - 1; i++)
					{
						if (TerrainLayers[i].Start <= Z && Z < TerrainLayers[i + 1].Start)
						{
							const uint8 Alpha = FMath::Clamp<int>(255 * (TerrainLayers[i + 1].Start - 1 - Z) / FadeHeight, 0, 255);

							// Alternate first material to avoid problem with alpha smoothing
							if (i % 2 == 0)
							{
								Material = FVoxelMaterial(TerrainLayers[i + 1].Material, TerrainLayers[i].Material, Alpha);
							}
							else
							{
								Material = FVoxelMaterial(TerrainLayers[i].Material, TerrainLayers[i + 1].Material, 255 - Alpha);
							}
						}
					}
				}
			}
		}
		for (int J = 0; J < Size.Y; J++)
		{
			const int Y = Start.Y + J * Step;
			for (int I = 0; I < Size.X; I++)
			{
				const int X = Start.X + I * Step;
				const int Index = (StartIndex.X + I) + ArraySize.X * (StartIndex.Y + J) + ArraySize.X * ArraySize.Y * (StartIndex.Z + K);

				if (Values)
				{
					Values[Index] = Value;
				}
				if (Materials)
				{
					Materials[Index] = Material;
				}
			}
		}
	}
}

void UFlatWorldGenerator::SetVoxelWorld(AVoxelWorld* VoxelWorld)
{
	TerrainLayers.Sort([](const FFlatWorldLayer& Left, const FFlatWorldLayer& Right) { return Left.Start < Right.Start; });
}
