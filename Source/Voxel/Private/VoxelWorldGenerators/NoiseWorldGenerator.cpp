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

	if (10000 < Start.Z)
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

					float Density = Z;

					{
						float x = X;
						float y = Y;
						float z = Z;

						Density -= FMath::Clamp<float>(10000 * Noise.GetSimplexFractal(x / 1000, y / 1000), 1000, 10000) - 1000;

						Density -= FMath::Clamp<float>(250 * Noise.GetSimplexFractal(x / 100, y / 100), 0, 250);
					}

					const float A = FMath::Lerp(0.25f, 2.f, FMath::Clamp(Z - 5.f, 0.f, 1.f));
					const float B = FMath::Lerp(0.f, 5.f, FMath::Clamp((5.f - Z) / 4.f, 0.f, 1.f));

					float ALayerValue = 0;

					if (Density + A + B > -2 || Density - A - B < 2)
					{
						{
							float x = X;
							float y = Y;
							float z = Z;

							Noise.GradientPerturb(x, y, z);

							ALayerValue = A * Noise.GetSimplexFractal(x, y, z);

							Density -= ALayerValue;
						}

						{
							float x = X;
							float y = Y;
							float z = Z;
							Density += B * FMath::Clamp(Noise.GetSimplex(x, y), 0.f, 1.f);
						}
					}

					const int Index = (StartIndex.X + I) + ArraySize.X * (StartIndex.Y + J) + ArraySize.X * ArraySize.Y * (StartIndex.Z + K);
					if (Values)
					{
						Values[Index] = FMath::Clamp(Density, -2.f, 2.f) / 2.f;
					}
					if (Materials)
					{
						FVoxelMaterial Material;
						if (Z < -10)
						{
							Material = FVoxelMaterial(0, 0, 0);
						}
						else if (Z < 10)
						{
							Material = FVoxelMaterial(0, 1, FMath::Clamp<int>(Z * 256.f / 10.f, 0, 255));
						}
						else if (Z < 11)
						{
							Material = FVoxelMaterial(1, 1, 255);
						}
						else if (Z < 12)
						{
							Material = FVoxelMaterial(1, 2, 0);
						}
						else if (Z < 1000)
						{
							Material = FVoxelMaterial(1, 2, FMath::Clamp<int>(-ALayerValue * 256.f, 0, 255));
						}
						else if (Z < 1500)
						{
							Material = FVoxelMaterial(1, 3, FMath::Clamp<int>((Z - 1000.f) * 256.f / 500.f, 0, 255));
						}
						else
						{
							Material = FVoxelMaterial(3, 3, 255);
						}
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