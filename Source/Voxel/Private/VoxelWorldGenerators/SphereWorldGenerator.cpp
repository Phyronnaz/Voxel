// Copyright 2017 Phyronnaz

#include "SphereWorldGenerator.h"

USphereWorldGenerator::USphereWorldGenerator() : Radius(1000), InverseOutsideInside(false), HardnessMultiplier(1)
{

}



void USphereWorldGenerator::GetValuesAndMaterials(float Values[], FVoxelMaterial Materials[], const FIntVector& Start, const FIntVector& StartIndex, const int Step, const FIntVector& Size, const FIntVector& ArraySize) const
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


				const int Index = (StartIndex.X + I) + ArraySize.X * (StartIndex.Y + J) + ArraySize.X * ArraySize.Y * (StartIndex.Z + K);
				if (Values)
				{
					// Distance to the center
					float Distance = FVector(X, Y, Z).Size();

					// Alpha = -1 inside the sphere, 1 outside and an interpolated value on intersection
					float Alpha = FMath::Clamp(Distance - LocalRadius, -2.f, 2.f) / 2;

					// Multiply by custom HardnessMultiplier (allows for rocks harder to mine)
					Alpha *= HardnessMultiplier;

					Values[Index] = Alpha * (InverseOutsideInside ? -1 : 1);
				}
				if (Materials)
				{
					Materials[Index] = DefaultMaterial;
				}
			}
		}
	}
}

void USphereWorldGenerator::SetVoxelWorld(AVoxelWorld* VoxelWorld)
{
	LocalRadius = Radius / VoxelWorld->GetVoxelSize();
}

FVector USphereWorldGenerator::GetUpVector(int X, int Y, int Z) const
{
	return FVector(X, Y, Z).GetSafeNormal();
}
