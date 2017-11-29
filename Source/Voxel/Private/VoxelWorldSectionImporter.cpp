// Copyright 2017 Phyronnaz

#include "VoxelWorldSectionImporter.h"
#include "VoxelWorld.h"
#include "VoxelData.h"
#include "DrawDebugHelpers.h"


AVoxelWorldSectionImporter::AVoxelWorldSectionImporter()
	: World(nullptr)
	, TopCorner(1, 1, 1)
	, BottomCorner(-1, -1, -1)
{
	PrimaryActorTick.bCanEverTick = true;
};

void AVoxelWorldSectionImporter::ImportToAsset(FDecompressedVoxelDataAsset& Asset)
{
	check(World);
	FIntVector Size = TopCorner - BottomCorner;
	Asset.SetHalfSize(FMath::CeilToInt(Size.X / 2), FMath::CeilToInt(Size.Y / 2), FMath::CeilToInt(Size.Z / 2));

	FVoxelData* Data = World->GetData();

	FVoxelBox Bounds = Asset.GetBounds();

	{
		Data->BeginGet();
		for (int X = Bounds.Min.X; X <= Bounds.Max.X; X++)
		{
			for (int Y = Bounds.Min.Y; Y <= Bounds.Max.Y; Y++)
			{
				for (int Z = Bounds.Min.Z; Z <= Bounds.Max.Z; Z++)
				{
					FIntVector P = BottomCorner + FIntVector(X, Y, Z) - Bounds.Min;
					if (LIKELY(Data->IsInWorld(P.X, P.Y, P.Z)))
					{
						float Value;
						FVoxelMaterial Material;
						Data->GetValueAndMaterial(P.X, P.Y, P.Z, Value, Material);

						Asset.SetValue(X, Y, Z, Value);
						Asset.SetMaterial(X, Y, Z, Material);
						Asset.SetVoxelType(X, Y, Z, FVoxelType(Value > 1 - KINDA_SMALL_NUMBER ? IgnoreValue : (Value >= 0 ? UseValueIfSameSign : UseValue), Value < 1 - KINDA_SMALL_NUMBER ? UseMaterial : IgnoreMaterial));
					}
					else
					{
						Asset.SetValue(X, Y, Z, 0);
						Asset.SetMaterial(X, Y, Z, FVoxelMaterial(0, 0, 0));
						Asset.SetVoxelType(X, Y, Z, FVoxelType(IgnoreValue, IgnoreMaterial));
					}
				}
			}
		}
		Data->BeginGet();
	}
}

void AVoxelWorldSectionImporter::SetCornersFromActors()
{
	check(World);
	if (BottomActor)
	{
		BottomCorner = World->GlobalToLocal(BottomActor->GetActorLocation());
	}
	if (TopActor)
	{
		TopCorner = World->GlobalToLocal(TopActor->GetActorLocation());
	}
	if (BottomCorner.X > TopCorner.X)
	{
		BottomCorner.X = TopCorner.X;
	}
	if (BottomCorner.Y > TopCorner.Y)
	{
		BottomCorner.Y = TopCorner.Y;
	}
	if (BottomCorner.Z > TopCorner.Z)
	{
		BottomCorner.Z = TopCorner.Z;
	}
}

#if WITH_EDITOR
void AVoxelWorldSectionImporter::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (BottomCorner.X > TopCorner.X)
	{
		BottomCorner.X = TopCorner.X;
	}
	if (BottomCorner.Y > TopCorner.Y)
	{
		BottomCorner.Y = TopCorner.Y;
	}
	if (BottomCorner.Z > TopCorner.Z)
	{
		BottomCorner.Z = TopCorner.Z;
	}
}

void AVoxelWorldSectionImporter::Tick(float Deltatime)
{
	if (World)
	{
		FVector Bottom = World->LocalToGlobal(BottomCorner);
		FVector Top = World->LocalToGlobal(TopCorner);
		DrawDebugBox(GetWorld(), (Bottom + Top) / 2, (Top - Bottom) / 2, FColor::Red, false, 2 * Deltatime, 0, 10);
	}
}
#endif
