// Copyright 2017 Phyronnaz

#include "VoxelAssetWorldGenerator.h"
#include "VoxelPrivate.h"
#include "EmptyWorldGenerator.h"

UVoxelAssetWorldGenerator::UVoxelAssetWorldGenerator()
	: InstancedWorldGenerator(nullptr)
	, DecompressedAsset(nullptr)
{
	DefaultWorldGenerator = TSubclassOf<UVoxelWorldGenerator>(UEmptyWorldGenerator::StaticClass());
}

UVoxelAssetWorldGenerator::~UVoxelAssetWorldGenerator()
{
	delete DecompressedAsset;
}

void UVoxelAssetWorldGenerator::GetValuesAndMaterials(float Values[], FVoxelMaterial Materials[], const FIntVector& Start, const FIntVector& StartIndex, const int Step, const FIntVector& Size, const FIntVector& ArraySize) const
{
	if (Bounds.Intersect(FVoxelBox(Start, Start + (Size - FIntVector(1, 1, 1)) * Step)))
	{
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
						if (Bounds.IsInside(X, Y, Z))
						{
							float AssetValue = DecompressedAsset->GetValue(X, Y, Z);
							EVoxelValueType ValueType = DecompressedAsset->GetVoxelType(X, Y, Z).GetValueType();

							if (ValueType == UseValue)
							{
								Values[Index] = AssetValue;
							}
							else
							{
								float DefaultValue = InstancedWorldGenerator->GetValue(X, Y, Z);
								if ((ValueType == UseValueIfSameSign && FVoxelType::HaveSameSign(DefaultValue, AssetValue)) ||
									(ValueType == UseValueIfDifferentSign && !FVoxelType::HaveSameSign(DefaultValue, AssetValue)))
								{
									Values[Index] = AssetValue;
								}
								else
								{
									Values[Index] = DefaultValue;
								}
							}
						}
						else
						{
							Values[Index] = InstancedWorldGenerator->GetValue(X, Y, Z);
						}
					}
					if (Materials)
					{
						if (Bounds.IsInside(X, Y, Z))
						{
							EVoxelMaterialType MaterialType = DecompressedAsset->GetVoxelType(X, Y, Z).GetMaterialType();

							if (MaterialType == UseMaterial)
							{
								Materials[Index] = DecompressedAsset->GetMaterial(X, Y, Z);
							}
							else
							{
								Materials[Index] = InstancedWorldGenerator->GetMaterial(X, Y, Z);
							}
						}
						else
						{
							Materials[Index] = InstancedWorldGenerator->GetMaterial(X, Y, Z);
						}
					}
				}
			}
		}
	}
	else
	{
		InstancedWorldGenerator->GetValuesAndMaterials(Values, Materials, Start, StartIndex, Step, Size, ArraySize);
	}
}

void UVoxelAssetWorldGenerator::SetVoxelWorld(AVoxelWorld* VoxelWorld)
{
	CreateGeneratorAndDecompressedAsset(VoxelWorld->GetVoxelSize());
	InstancedWorldGenerator->SetVoxelWorld(VoxelWorld);
}

void UVoxelAssetWorldGenerator::CreateGeneratorAndDecompressedAsset(const float VoxelSize)
{
	check(!InstancedWorldGenerator);

	if (DefaultWorldGenerator)
	{
		InstancedWorldGenerator = NewObject<UVoxelWorldGenerator>((UObject*)GetTransientPackage(), DefaultWorldGenerator);
	}
	if (InstancedWorldGenerator == nullptr)
	{
		UE_LOG(LogVoxel, Error, TEXT("VoxelAssetWorldGenerator: Invalid world generator"));
		InstancedWorldGenerator = NewObject<UVoxelWorldGenerator>((UObject*)GetTransientPackage(), UEmptyWorldGenerator::StaticClass());
	}

	bool bSuccess = Asset && Asset->GetDecompressedAsset(DecompressedAsset, VoxelSize);

	if (bSuccess)
	{
		Bounds = DecompressedAsset->GetBounds();
	}
	else
	{
		Bounds.Min = FIntVector(2, 2, 2);
		Bounds.Max = FIntVector(-2, -2, -2);
		UE_LOG(LogVoxel, Error, TEXT("VoxelAssetWorldGenerator: Invalid asset"));
	}
}
