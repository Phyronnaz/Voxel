// Copyright 2017 Phyronnaz

#pragma once

#include "NoiseWorldGenerator.h"

UNoiseWorldGenerator::UNoiseWorldGenerator()
	: Noise()
{
}

void UNoiseWorldGenerator::GetValuesAndMaterials(float Values[], FVoxelMaterial Materials[], const FIntVector& Start, const FIntVector& StartIndex, const int Step, const FIntVector& Size, const FIntVector& ArraySize) const
{
	check(Start.X % Step == 0);
	check(Start.Y % Step == 0);
	check(Start.Z % Step == 0);

	if (1000 < Start.Z)
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
						Materials[Index] = FVoxelMaterial(3, 3, 255);
					}
				}
			}
		}
	}
	else if (Start.Z + Size.Z * Step < -100)
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
						Values[Index] = -1;
					}
					if (Materials)
					{
						Materials[Index] = FVoxelMaterial(0, 0, 0);
					}
				}
			}
		}
	}
	else
	{
		for (int K = 0; K < Size.Z; K++)
		{
			const int Z = Start.Z + K * Step;

			for (int J = 0; J < Size.Y; J++)
			{
				const int Y = Start.Y + J * Step;
				for (int I = 0; I < Size.X; I++)
				{
					const int X = Start.X + I * Step;

					const int NoiseIndex = K + Size.Z * J + Size.Z * Size.Y * I;

					float Density = FMath::Min(Z, 0);

					float x = X;
					float y = Y;
					float z = Z;
					Density += 10 * Noise.GetSimplexFractal(x, y, z);

					Density = FMath::Lerp(Density, 2.f, 1.f - FMath::Clamp((10.f - Z) / 2.5f, 0.f, 1.f));

					const int Index = (StartIndex.X + I) + ArraySize.X * (StartIndex.Y + J) + ArraySize.X * ArraySize.Y * (StartIndex.Z + K);
					if (Values)
					{
						Values[Index] = FMath::Clamp(Density, -2.f, 2.f) / 2.f;
					}
					if (Materials)
					{
						FVoxelMaterial Material;
						Material = FVoxelMaterial(0, 0, 0);
						Materials[Index] = Material;
					}
				}
			}
		}
	}
}

void UNoiseWorldGenerator::SetVoxelWorld(AVoxelWorld* VoxelWorld)
{
	Noise.SetGradientPerturbAmp(45);
	Noise.SetFrequency(0.02);
};