// Copyright 2017 Phyronnaz

#pragma once

#include "VoxelPrivatePCH.h"
#include "CoreMinimal.h"
#include "PerlinNoiseWorldGenerator.h"

UPerlinNoiseWorldGenerator::UPerlinNoiseWorldGenerator() : Noise()
{
}

float UPerlinNoiseWorldGenerator::GetDefaultValue(int X, int Y, int Z)
{
	const float R = FVector(X, Y, Z).Size();
	const float Height = R - 100000;

	if (Height < -100)
	{
		return -1;
	}
	else if (10000 < Height)
	{
		return 1;
	}
	else
	{
		const float Phi = FMath::Atan2(Y, X) * 100000;
		const float Theta = FMath::Acos(Z / R) * 100000;

		float Density = Height;

		{
			float x = Phi;
			float y = Theta;

			Density -= FMath::Clamp<float>(10000 * Noise.GetSimplexFractal(x / 1000, y / 1000), 1000, 10000) - 1000;

			Density -= FMath::Clamp<float>(250 * Noise.GetSimplexFractal(x / 100, y / 100), 0, 250);
		}

		const float A = FMath::Lerp(0.25f, 2.f, FMath::Clamp(Height - 5.f, 0.f, 1.f));
		{
			float x = X;
			float y = Y;
			float z = Z;

			Noise.GradientPerturb(x, y, z);

			Density -= A * Noise.GetSimplexFractal(x, y, z);
		}

		const float B = FMath::Lerp(0.f, 5.f, FMath::Clamp((5.f - Height) / 4.f, 0.f, 1.f));
		{
			float x = Phi;
			float y = Theta;
			Density += B * FMath::Clamp(Noise.GetSimplex(x, y), 0.f, 1.f);
		}

		return FMath::Clamp(Density, -2.f, 2.f) / 2.f;
	}
}

FVoxelMaterial UPerlinNoiseWorldGenerator::GetDefaultMaterial(int X, int Y, int Z)
{
	const float R = FVector(X, Y, Z).Size();
	const float Height = R - 100000;

	if (Height < -10)
	{
		return FVoxelMaterial(0, 0, 0);
	}
	else if (Height < 10)
	{
		return FVoxelMaterial(0, 1, FMath::Clamp<int>(Height * 256.f / 10.f, 0, 255));
	}
	else if (Height < 11)
	{
		return FVoxelMaterial(1, 1, 255);
	}
	else if (Height < 12)
	{
		return FVoxelMaterial(1, 2, 0);
	}
	else if (Height < 1000)
	{
		const float A = FMath::Lerp(0.25f, 2.f, FMath::Clamp(Height - 5.f, 0.f, 1.f));
		float Density;
		{
			float x = X;
			float y = Y;
			float z = Z;

			Noise.GradientPerturb(x, y, z);

			Density = A * Noise.GetSimplexFractal(x, y, z);
		}

		return FVoxelMaterial(1, 2, FMath::Clamp<int>(-Density * 256.f, 0, 255));
	}
	else if (Height < 1500)
	{
		return FVoxelMaterial(1, 3, FMath::Clamp<int>((Height - 1000.f) * 256.f / 500.f, 0, 255));
	}
	else
	{
		return FVoxelMaterial(3, 3, 255);
	}
}

void UPerlinNoiseWorldGenerator::SetVoxelWorld(AVoxelWorld* VoxelWorld)
{
	Noise = FastNoise();
	Noise.SetGradientPerturbAmp(45);
	Noise.SetFrequency(0.02);
};

FVector UPerlinNoiseWorldGenerator::GetUpVector(int X, int Y, int Z)
{
	return FVector(X, Y, Z).GetSafeNormal();
}
