// Copyright 2017 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelMaterial.h"
#include "VoxelWorldGenerator.generated.h"

class AVoxelWorld;

// TODO: Remove UObject with intermediate class

/**
 *
 */
UCLASS(Blueprintable, abstract, HideCategories = ("Tick", "Replication", "Input", "Actor", "Rendering"))
class VOXEL_API UVoxelWorldGenerator : public UObject
{
	GENERATED_BODY()

public:

	float GetValue(int X, int Y, int Z) const
	{
		float Values[1];
		GetValuesAndMaterials(Values, nullptr, FIntVector(X, Y, Z), FIntVector::ZeroValue, 1, FIntVector(1, 1, 1), FIntVector(1, 1, 1));
		return Values[0];
	}
	FVoxelMaterial GetMaterial(int X, int Y, int Z) const
	{
		FVoxelMaterial Materials[1];
		GetValuesAndMaterials(nullptr, Materials, FIntVector(X, Y, Z), FIntVector::ZeroValue, 1, FIntVector(1, 1, 1), FIntVector(1, 1, 1));
		return Materials[0];
	}

	/**
	 * Values or Materials can be null
	 * Start.X <= X < Step * Size.X
	 * Start % Step == 0
	 *
	 * @param	Values		Values. Can be nullptr
	 * @param	Materials	Materials. Can be nullptr
	 * @param	Start		Start of the processed chunk in voxel position
	 * @param	StartIndex	Start of the processed chunk in the arrays
	 * @param	Step		How far are each values from each other in voxel position
	 * @param	Size		Size of the chunk in the arrays. Voxel Size of the chunk = Size * Step
	 * @param	ArraySize	Size of the arrays
	 */
	virtual void GetValuesAndMaterials(float Values[], FVoxelMaterial Materials[], const FIntVector& Start, const FIntVector& StartIndex, const int Step, const FIntVector& Size, const FIntVector& ArraySize) const
	{
		check(Start.X % Step == 0);
		check(Start.Y % Step == 0);
		check(Start.Z % Step == 0);

		for (int K = 0; K < Size.Z; K++)
		{
			const int Z = Start.Z + K * Step;
			// If Value doesn't depend on X and Y, you should compute it here

			for (int J = 0; J < Size.Y; J++)
			{
				const int Y = Start.Y + J * Step;
				// If Value doesn't depend on X, you should compute it here

				for (int I = 0; I < Size.X; I++)
				{
					const int X = Start.X + I * Step;


					const int Index = (StartIndex.X + I) + ArraySize.X * (StartIndex.Y + J) + ArraySize.X * ArraySize.Y * (StartIndex.Z + K);
					if (Values)
					{
						Values[Index] = 0; // Your value here
					}
					if (Materials)
					{
						Materials[Index] = FVoxelMaterial(); // Your material here
					}
				}
			}
		}
	}

	/**
	 * If you need a reference to Voxel World
	 */
	virtual void SetVoxelWorld(AVoxelWorld* VoxelWorld)
	{

	}

	/**
	 * World up vector at position
	 */
	virtual FVector GetUpVector(int X, int Y, int Z) const
	{
		return FVector::UpVector;
	}
};
