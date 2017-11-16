// Copyright 2017 Phyronnaz

#pragma once

#include "PerlinNoiseWorldGenerator.h"
#include "CoreMinimal.h"
#include "VoxelPrivatePCH.h"

UPerlinNoiseWorldGenerator::UPerlinNoiseWorldGenerator() : Noise()
{
}

void UPerlinNoiseWorldGenerator::GetValuesAndMaterials(float Values[], FVoxelMaterial Materials[], const FIntVector& Start, const FIntVector& StartIndex, const int Step, const FIntVector& Size, const FIntVector& ArraySize) const
{
	float* NoiseValues = NoiseSIMD->GetSimplexFractalSet(Start.X, Start.Y, Start.Z, Size.X, Size.Y, Size.Z, Step);

	for (int K = 0; K < Size.Z; K++)
	{
		const int Z = Start.Z + K * Step;

		for (int J = 0; J < Size.Y; J++)
		{
			const int Y = Start.Y + J * Step;
			for (int I = 0; I < Size.X; I++)
			{
				const int X = Start.X + I * Step;

				float Density = Z;

				//Density += 10 * Noise.GetSimplexFractal(x, y);

				Density -= 10 * NoiseValues[K + Size.Z * J + Size.Z * Size.Y * I];

				const int Index = (StartIndex.X + I) + ArraySize.X * (StartIndex.Y + J) + ArraySize.X * ArraySize.Y * (StartIndex.Z + K);
				if (Values)
				{
					Values[Index] = Density;
				}
				if (Materials)
				{
					//Materials[Index] = Material;
				}
			}
		}
	}
	FastNoiseSIMD::FreeNoiseSet(NoiseValues);
}

//float UPerlinNoiseWorldGenerator::GetDefaultValue(int X, int Y, int Z)
//{
//	if (Z < -100)
//	{
//		return -1;
//	}
//	else if (10000 < Z)
//	{
//		return 1;
//	}
//	else
//	{
//		float Density = Z;
//
//		{
//			float x = X;
//			float y = Y;
//			float z = Z;
//
//			Density -= FMath::Clamp<float>(10000 * Noise.GetSimplexFractal(x / 1000, y / 1000), 1000, 10000) - 1000;
//
//			Density -= FMath::Clamp<float>(250 * Noise.GetSimplexFractal(x / 100, y / 100), 0, 250);
//		}
//
//		const float A = FMath::Lerp(0.25f, 2.f, FMath::Clamp(Z - 5.f, 0.f, 1.f));
//		{
//			float x = X;
//			float y = Y;
//			float z = Z;
//
//			Noise.GradientPerturb(x, y, z);
//
//			Density -= A * Noise.GetSimplexFractal(x, y, z);
//		}
//
//		const float B = FMath::Lerp(0.f, 100.f, FMath::Clamp((100.f - Z) / 100.f, 0.f, 1.f));
//		{
//			float x = X;
//			float y = Y;
//			float z = Z;
//			Density += B * FMath::Clamp(Noise.GetSimplexFractal(x, y, z), -1.f, 1.f);
//		}
//
//		return FMath::Clamp(Density, -2.f, 2.f) / 2.f;
//	}
//}
//
//FVoxelMaterial UPerlinNoiseWorldGenerator::GetDefaultMaterial(int X, int Y, int Z)
//{
//	if (Z < -10)
//	{
//		return FVoxelMaterial(0, 0, 0);
//	}
//	else if (Z < 10)
//	{
//		return FVoxelMaterial(0, 1, FMath::Clamp<int>(Z * 256.f / 10.f, 0, 255));
//	}
//	else if (Z < 11)
//	{
//		return FVoxelMaterial(1, 1, 255);
//	}
//	else if (Z < 12)
//	{
//		return FVoxelMaterial(1, 2, 0);
//	}
//	else if (Z < 1000)
//	{
//		const float A = FMath::Lerp(0.25f, 2.f, FMath::Clamp(Z - 5.f, 0.f, 1.f));
//		float Density;
//		{
//			float x = X;
//			float y = Y;
//			float z = Z;
//
//			Noise.GradientPerturb(x, y, z);
//
//			Density = A * Noise.GetSimplexFractal(x, y, z);
//		}
//
//		return FVoxelMaterial(1, 2, FMath::Clamp<int>(-Density * 256.f, 0, 255));
//	}
//	else if (Z < 1500)
//	{
//		return FVoxelMaterial(1, 3, FMath::Clamp<int>((Z - 1000.f) * 256.f / 500.f, 0, 255));
//	}
//	else
//	{
//		return FVoxelMaterial(3, 3, 255);
//	}
//}

void UPerlinNoiseWorldGenerator::SetVoxelWorld(AVoxelWorld* VoxelWorld)
{
	NoiseSIMD = FastNoiseSIMD::NewFastNoiseSIMD();
	NoiseSIMD->SetFrequency(0.02);

	Noise.SetGradientPerturbAmp(45);
	Noise.SetFrequency(0.02);
};