// Copyright 2017 Phyronnaz

#pragma once

#include "SphericalNoiseWorldGenerator.h"

USphericalNoiseWorldGenerator::USphericalNoiseWorldGenerator()
	: Noise()
	, Radius(1000)
{
}

void USphericalNoiseWorldGenerator::GetValuesAndMaterials(float Values[], FVoxelMaterial Materials[], const FIntVector& Start, const FIntVector& StartIndex, const int Step, const FIntVector& Size, const FIntVector& ArraySize) const
{
	check(Start.X % Step == 0);
	check(Start.Y % Step == 0);
	check(Start.Z % Step == 0);


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

				const float R = FVector(X, Y, Z).Size();
				const float Height = R - Radius;

				float Value;

				float ALayerValue = 0;


				if (Height < -100)
				{
					Value = -1;
				}
				else if (10000 < Height)
				{
					Value = 1;
				}
				else
				{
					/*const float Phi = FMath::Atan2(Y, X) * Radius;
					const float Theta = FMath::Acos(Z / R) * Radius;*/

					const float NX = X * Radius / R;
					const float NY = Y * Radius / R;

					const float Sign = InverseInsideOutside ? -1 : 1;

					float Density = Height;

					{
						float x = NX;
						float y = NY;

						Density -= Sign * (FMath::Clamp<float>(10000 * Noise.GetSimplexFractal(x / 1000, y / 1000), 1000, 10000) - 1000);

						Density -= Sign * FMath::Clamp<float>(250 * Noise.GetSimplexFractal(x / 100, y / 100), 0, 250);
					}

					const float A = FMath::Lerp(0.25f, 2.f, FMath::Clamp(Height - 5.f, 0.f, 1.f));
					const float B = FMath::Lerp(0.f, 5.f, FMath::Clamp((5.f - Height) / 4.f, 0.f, 1.f));

					if (Density + A + B > -2 || Density - A - B < 2)
					{
						{
							float x = X;
							float y = Y;
							float z = Z;

							Noise.GradientPerturb(x, y, z);

							ALayerValue = Sign * A * Noise.GetSimplexFractal(x, y, z);

							Density -= ALayerValue;
						}

						{
							float x = NX;
							float y = NY;
							Density += Sign * B * FMath::Clamp(Noise.GetSimplex(x, y), 0.f, 1.f);
						}
					}

					Value = FMath::Clamp(Density, -2.f, 2.f) / 2.f;
				}

				const int Index = (StartIndex.X + I) + ArraySize.X * (StartIndex.Y + J) + ArraySize.X * ArraySize.Y * (StartIndex.Z + K);
				if (Values)
				{
					Values[Index] = Value * (InverseInsideOutside ? -1 : 1);
				}
				if (Materials)
				{
					const int NewHeight = (Height / Step) * Step;
					FVoxelMaterial Material;
					if (NewHeight < -10)
					{
						Material = FVoxelMaterial(0, 0, 0);
					}
					else if (NewHeight < 10)
					{
						Material = FVoxelMaterial(0, 1, FMath::Clamp<int>(NewHeight * 256.f / 10.f, 0, 255));
					}
					else if (NewHeight < 11)
					{
						Material = FVoxelMaterial(1, 1, 255);
					}
					else if (NewHeight < 12)
					{
						Material = FVoxelMaterial(1, 2, 0);
					}
					else if (NewHeight < 1000)
					{
						Material = FVoxelMaterial(1, 2, FMath::Clamp<int>(-ALayerValue * 256.f, 0, 255));
					}
					else if (NewHeight < 1500)
					{
						Material = FVoxelMaterial(1, 3, FMath::Clamp<int>((NewHeight - 1000.f) * 256.f / 500.f, 0, 255));
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

void USphericalNoiseWorldGenerator::SetVoxelWorld(AVoxelWorld* VoxelWorld)
{
	Noise.SetGradientPerturbAmp(45);
	Noise.SetFrequency(0.02);
};

FVector USphericalNoiseWorldGenerator::GetUpVector(int X, int Y, int Z) const
{
	return FVector(X, Y, Z).GetSafeNormal();
}
