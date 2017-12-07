// Copyright 2017 Phyronnaz

#include "VoxelTools.h"
#include "VoxelPrivate.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/HUD.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "EmptyWorldGenerator.h"
#include "VoxelData.h"
#include "FastNoise.h"
#include "Misc/QueuedThreadPool.h"

DECLARE_CYCLE_STAT(TEXT("VoxelTool ~ SetValueSphere"), STAT_SetValueSphere, STATGROUP_Voxel);
DECLARE_CYCLE_STAT(TEXT("VoxelTool ~ SetMaterialSphere"), STAT_SetMaterialSphere, STATGROUP_Voxel);

DECLARE_CYCLE_STAT(TEXT("VoxelTool ~ SetValueProjection"), STAT_SetValueProjection, STATGROUP_Voxel);
DECLARE_CYCLE_STAT(TEXT("VoxelTool ~ SetMaterialProjection"), STAT_SetMaterialProjection, STATGROUP_Voxel);

DECLARE_CYCLE_STAT(TEXT("VoxelTool ~ SmoothValue"), STAT_SmoothValue, STATGROUP_Voxel);


FAsyncAddCrater::FAsyncAddCrater(FVoxelData* Data, const FIntVector LocalPosition, const int IntRadius, const float Radius, const uint8 BlackMaterialIndex, const uint8 AddedBlack, const float HardnessMultiplier)
	:Data(Data)
	, LocalPosition(LocalPosition)
	, IntRadius(IntRadius)
	, Radius(Radius)
	, BlackMaterialIndex(BlackMaterialIndex)
	, AddedBlack(AddedBlack)
	, HardnessMultiplier(HardnessMultiplier)
{

}

void FAsyncAddCrater::DoThreadedWork()
{
	FValueOctree* LastOctree = nullptr;
	FastNoise Noise;
	Data->BeginSet();
	for (int X = -IntRadius; X <= IntRadius; X++)
	{
		for (int Y = -IntRadius; Y <= IntRadius; Y++)
		{
			for (int Z = -IntRadius; Z <= IntRadius; Z++)
			{
				const FIntVector CurrentPosition = LocalPosition + FIntVector(X, Y, Z);

				float CurrentRadius = FVector(X, Y, Z).Size();
				float Distance = CurrentRadius;

				if (Radius - 2 < Distance && Distance <= Radius + 3)
				{
					float CurrentNoise = Noise.GetWhiteNoise(X / CurrentRadius, Y / CurrentRadius, Z / CurrentRadius);
					Distance -= CurrentNoise;
				}

				if (Distance <= Radius + 2)
				{
					// We want (Radius - Distance) != 0
					const float Noise = (Radius - Distance == 0) ? 0.0001f : 0;
					float Value = FMath::Clamp(Radius - Distance + Noise, -2.f, 2.f) / 2;

					Value *= HardnessMultiplier;

					float OldValue;
					FVoxelMaterial OldMaterial;
					Data->GetValueAndMaterial(CurrentPosition.X, CurrentPosition.Y, CurrentPosition.Z, OldValue, OldMaterial);

					bool bValid;
					if (Value > 0)
					{
						bValid = true;
					}
					else
					{
						bValid = (OldValue > 0 && Value > 0) || (OldValue <= 0 && Value <= 0);
					}
					if (bValid)
					{
						if (LIKELY(Data->IsInWorld(CurrentPosition.X, CurrentPosition.Y, CurrentPosition.Z)))
						{
							if (OldMaterial.Index1 == BlackMaterialIndex)
							{
								OldMaterial.Alpha = FMath::Clamp<int>(OldMaterial.Alpha - AddedBlack, 0, 255);
							}
							else if (OldMaterial.Index2 == BlackMaterialIndex)
							{
								OldMaterial.Alpha = FMath::Clamp<int>(OldMaterial.Alpha + AddedBlack, 0, 255);
							}
							else if (OldMaterial.Alpha < 128)
							{
								// Index 1 biggest
								OldMaterial.Index2 = BlackMaterialIndex;
								OldMaterial.Alpha = FMath::Clamp<int>(AddedBlack, 0, 255);
							}
							else
							{
								// Index 2 biggest
								OldMaterial.Index1 = BlackMaterialIndex;
								OldMaterial.Alpha = FMath::Clamp<int>(255 - AddedBlack, 0, 255);
							}

							Data->SetValueAndMaterial(CurrentPosition.X, CurrentPosition.Y, CurrentPosition.Z, Value, OldMaterial, LastOctree);
						}
					}
				}
			}
		}
	}
	Data->EndSet();

	delete this;
}

void FAsyncAddCrater::Abandon()
{
	delete this;
}




void UVoxelTools::AddCrater(AVoxelWorld* World, const FVector Position, const float WorldRadius, const uint8 BlackMaterialIndex, const uint8 AddedBlack, const bool bAsync /*= false*/, const float HardnessMultiplier /*= 1*/)
{
	if (!World)
	{
		UE_LOG(LogVoxel, Error, TEXT("SetValueSphere: World is NULL"));
		return;
	}

	const float Radius = WorldRadius / World->GetVoxelSize();

	// Position in voxel space
	FIntVector LocalPosition = World->GlobalToLocal(Position);
	int IntRadius = FMath::CeilToInt(Radius) + 2;

	FValueOctree* LastOctree = nullptr;
	FVoxelData* Data = World->GetData();

	FastNoise Noise;

	{
		Data->BeginSet();
		for (int X = -IntRadius; X <= IntRadius; X++)
		{
			for (int Y = -IntRadius; Y <= IntRadius; Y++)
			{
				for (int Z = -IntRadius; Z <= IntRadius; Z++)
				{
					const FIntVector CurrentPosition = LocalPosition + FIntVector(X, Y, Z);

					float CurrentRadius = FVector(X, Y, Z).Size();
					float Distance = CurrentRadius;

					if (Radius - 2 < Distance && Distance <= Radius + 3)
					{
						float CurrentNoise = Noise.GetWhiteNoise(X / CurrentRadius, Y / CurrentRadius, Z / CurrentRadius);
						Distance -= CurrentNoise;
					}

					if (Distance <= Radius + 2)
					{
						// We want (Radius - Distance) != 0
						const float Noise = (Radius - Distance == 0) ? 0.0001f : 0;
						float Value = FMath::Clamp(Radius - Distance + Noise, -2.f, 2.f) / 2;

						Value *= HardnessMultiplier;

						float OldValue;
						FVoxelMaterial OldMaterial;
						Data->GetValueAndMaterial(CurrentPosition.X, CurrentPosition.Y, CurrentPosition.Z, OldValue, OldMaterial);

						bool bValid;
						if (Value > 0)
						{
							bValid = true;
						}
						else
						{
							bValid = (OldValue > 0 && Value > 0) || (OldValue <= 0 && Value <= 0);
						}
						if (bValid)
						{
							if (LIKELY(Data->IsInWorld(CurrentPosition.X, CurrentPosition.Y, CurrentPosition.Z)))
							{
								if (OldMaterial.Index1 == BlackMaterialIndex)
								{
									OldMaterial.Alpha = FMath::Clamp<int>(OldMaterial.Alpha - AddedBlack, 0, 255);
								}
								else if (OldMaterial.Index2 == BlackMaterialIndex)
								{
									OldMaterial.Alpha = FMath::Clamp<int>(OldMaterial.Alpha + AddedBlack, 0, 255);
								}
								else if (OldMaterial.Alpha < 128)
								{
									// Index 1 biggest
									OldMaterial.Index2 = BlackMaterialIndex;
									OldMaterial.Alpha = FMath::Clamp<int>(AddedBlack, 0, 255);
								}
								else
								{
									// Index 2 biggest
									OldMaterial.Index1 = BlackMaterialIndex;
									OldMaterial.Alpha = FMath::Clamp<int>(255 - AddedBlack, 0, 255);
								}

								Data->SetValueAndMaterial(CurrentPosition.X, CurrentPosition.Y, CurrentPosition.Z, Value, OldMaterial, LastOctree);
							}
						}
					}
				}
			}
		}
		Data->EndSet();
	}
	World->UpdateChunksOverlappingBox(FVoxelBox(LocalPosition + FIntVector(1, 1, 1) * -(IntRadius + 1), LocalPosition + FIntVector(1, 1, 1) * (IntRadius + 1)), bAsync);
}

void UVoxelTools::AddCraterMultithreaded(AVoxelWorld* World, const FVector Position, const float WorldRadius, const uint8 BlackMaterialIndex, const uint8 AddedBlack /*= 150*/, const bool bAsync /*= false*/, const float HardnessMultiplier /*= 1*/)
{
	if (!World)
	{
		UE_LOG(LogVoxel, Error, TEXT("SetValueSphere: World is NULL"));
		return;
	}

	const float Radius = WorldRadius / World->GetVoxelSize();

	// Position in voxel space
	FIntVector LocalPosition = World->GlobalToLocal(Position);
	int IntRadius = FMath::CeilToInt(Radius) + 2;

	FVoxelData* Data = World->GetData();

	auto Task = new FAsyncAddCrater(Data, LocalPosition, IntRadius, Radius, BlackMaterialIndex, AddedBlack, HardnessMultiplier);
	GThreadPool->AddQueuedWork(Task);

	World->UpdateChunksOverlappingBox(FVoxelBox(LocalPosition + FIntVector(1, 1, 1) * -(IntRadius + 1), LocalPosition + FIntVector(1, 1, 1) * (IntRadius + 1)), bAsync);
}

void UVoxelTools::SetValueSphere(AVoxelWorld* World, const FVector Position, const float WorldRadius, const bool bAdd, const bool bAsync, const float HardnessMultiplier)
{
	SCOPE_CYCLE_COUNTER(STAT_SetValueSphere);

	if (!World)
	{
		UE_LOG(LogVoxel, Error, TEXT("SetValueSphere: World is NULL"));
		return;
	}
	const float Radius = WorldRadius / World->GetVoxelSize();

	// Position in voxel space
	FIntVector LocalPosition = World->GlobalToLocal(Position);
	int IntRadius = FMath::CeilToInt(Radius) + 2;

	FValueOctree* LastOctree = nullptr;
	FVoxelData* Data = World->GetData();

	{
		Data->BeginSet();
		for (int X = -IntRadius; X <= IntRadius; X++)
		{
			for (int Y = -IntRadius; Y <= IntRadius; Y++)
			{
				for (int Z = -IntRadius; Z <= IntRadius; Z++)
				{
					const FIntVector CurrentPosition = LocalPosition + FIntVector(X, Y, Z);
					const float Distance = FVector(X, Y, Z).Size();

					if (Distance <= Radius + 2)
					{
						// We want (Radius - Distance) != 0
						const float Noise = (Radius - Distance == 0) ? 0.0001f : 0;
						float Value = FMath::Clamp(Radius - Distance + Noise, -2.f, 2.f) / 2;

						Value *= HardnessMultiplier;
						Value *= (bAdd ? -1 : 1);

						float OldValue = Data->GetValue(CurrentPosition.X, CurrentPosition.Y, CurrentPosition.Z);

						bool bValid;
						if ((Value <= 0 && bAdd) || (Value > 0 && !bAdd))
						{
							bValid = true;
						}
						else
						{
							bValid = (OldValue > 0 && Value > 0) || (OldValue <= 0 && Value <= 0);
						}
						if (bValid)
						{
							if (LIKELY(Data->IsInWorld(CurrentPosition.X, CurrentPosition.Y, CurrentPosition.Z)))
							{
								Data->SetValue(CurrentPosition.X, CurrentPosition.Y, CurrentPosition.Z, Value, LastOctree);
							}
						}
					}
				}
			}
		}
		Data->EndSet();
	}
	World->UpdateChunksOverlappingBox(FVoxelBox(LocalPosition + FIntVector(1, 1, 1) * -(IntRadius + 1), LocalPosition + FIntVector(1, 1, 1) * (IntRadius + 1)), bAsync);
}

//void UVoxelTools::SetValueBox(AVoxelWorld* const World, const FVector Position, const float ExtentXInVoxel, const float ExtentYInVoxel, const float ExtentZInVoxel, const bool bAdd, const bool bAsync, const float HardnessMultiplier)
//{
//	if (World == nullptr)
//	{
//		UE_LOG(LogVoxel, Error, TEXT("World is NULL"));
//		return;
//	}
//	check(World);
//
//	FIntVector LocalPosition = World->GlobalToLocal(Position);
//	int IntExtentX = FMath::CeilToInt(ExtentXInVoxel);
//	int IntExtentY = FMath::CeilToInt(ExtentYInVoxel);
//	int IntHeight = FMath::CeilToInt(ExtentZInVoxel * 2);
//
//	float Value = HardnessMultiplier * (bAdd ? -1 : 1);
//
//	for (int X = -IntExtentX; X <= IntExtentX; X++)
//	{
//		for (int Y = -IntExtentY; Y <= IntExtentY; Y++)
//		{
//			for (int Z = 0; Z <= IntHeight; Z++)
//			{
//				FIntVector CurrentPosition = LocalPosition + FIntVector(X, Y, Z);
//
//				if ((Value < 0 && bAdd) || (Value >= 0 && !bAdd) || (World->GetValue(CurrentPosition) * Value > 0))
//				{
//					World->SetValue(CurrentPosition, Value);
//					World->UpdateChunksAtPosition(CurrentPosition, bAsync);
//				}
//			}
//		}
//	}
//}

void UVoxelTools::SetMaterialSphere(AVoxelWorld* World, const FVector Position, const float WorldRadius, const uint8 MaterialIndex, const bool bUseLayer1, const float FadeDistance, const bool bAsync)
{
	SCOPE_CYCLE_COUNTER(STAT_SetMaterialSphere);

	if (World == nullptr)
	{
		UE_LOG(LogVoxel, Error, TEXT("SetMaterialSphere: World is NULL"));
		return;
	}
	const float Radius = WorldRadius / World->GetVoxelSize();

	FIntVector LocalPosition = World->GlobalToLocal(Position);

	const float VoxelDiagonalLength = 1.73205080757f;
	const int Size = FMath::CeilToInt(Radius + FadeDistance + VoxelDiagonalLength);

	FValueOctree* LastOctree = nullptr;
	FVoxelData* Data = World->GetData();

	{
		Data->BeginSet();
		for (int X = -Size; X <= Size; X++)
		{
			for (int Y = -Size; Y <= Size; Y++)
			{
				for (int Z = -Size; Z <= Size; Z++)
				{
					const FIntVector CurrentPosition = LocalPosition + FIntVector(X, Y, Z);
					const float Distance = FVector(X, Y, Z).Size();


					FVoxelMaterial Material = Data->GetMaterial(CurrentPosition.X, CurrentPosition.Y, CurrentPosition.Z);

					if (Distance < Radius + FadeDistance + VoxelDiagonalLength)
					{
						// Set alpha
						int8 Alpha = 255 * FMath::Clamp((Radius + FadeDistance - Distance) / FadeDistance, 0.f, 1.f);
						if (bUseLayer1)
						{
							Alpha = 256 - Alpha;
						}
						if ((bUseLayer1 ? Material.Index1 : Material.Index2) == MaterialIndex)
						{
							// Same color
							Alpha = bUseLayer1 ? FMath::Min<uint8>(Alpha, Material.Alpha) : FMath::Max<uint8>(Alpha, Material.Alpha);
						}
						Material.Alpha = Alpha;

						// Set index
						if (bUseLayer1)
						{
							Material.Index1 = MaterialIndex;
						}
						else
						{
							Material.Index2 = MaterialIndex;
						}

						if (LIKELY(Data->IsInWorld(CurrentPosition.X, CurrentPosition.Y, CurrentPosition.Z)))
						{
							// Apply changes
							Data->SetMaterial(CurrentPosition.X, CurrentPosition.Y, CurrentPosition.Z, Material, LastOctree);
						}
					}
					else if (Distance < Radius + FadeDistance + 2 * VoxelDiagonalLength && (bUseLayer1 ? Material.Index1 : Material.Index2) != MaterialIndex)
					{
						Material.Alpha = bUseLayer1 ? 255 : 0;

						if (LIKELY(Data->IsInWorld(CurrentPosition.X, CurrentPosition.Y, CurrentPosition.Z)))
						{
							Data->SetMaterial(CurrentPosition.X, CurrentPosition.Y, CurrentPosition.Z, Material, LastOctree);
						}
					}
				}
			}
		}
		Data->EndSet();
	}
	World->UpdateChunksOverlappingBox(FVoxelBox(LocalPosition + FIntVector(1, 1, 1) * -(Size + 1), LocalPosition + FIntVector(1, 1, 1) * (Size + 1)), bAsync);
}


void FindModifiedPositionsForRaycasts(AVoxelWorld* World, const FVector StartPosition, const FVector Direction, const float Radius, const float MaxDistance, const float Precision,
	const bool bShowRaycasts, const bool bShowHitPoints, const bool bShowModifiedVoxels, std::deque<TTuple<FIntVector, float>>& OutModifiedPositionsAndDistances)
{
	const FVector ToolPosition = StartPosition - Direction * MaxDistance / 2;

	/**
	* Create a 2D basis from (Tangent, Bitangent)
	*/
	// Compute tangent
	FVector Tangent;
	// N dot T = 0
	// <=> N.X * T.X + N.Y * T.Y + N.Z * T.Z = 0
	// <=> T.Z = -1 / N.Z * (N.X * T.X + N.Y * T.Y) if N.Z != 0
	if (Direction.Z != 0)
	{
		Tangent.X = 1;
		Tangent.Y = 1;
		Tangent.Z = -1 / Direction.Z * (Direction.X * Tangent.X + Direction.Y * Tangent.Y);
	}
	else
	{
		Tangent = FVector(1, 0, 0);
	}
	Tangent.Normalize();

	// Compute bitangent
	const FVector Bitangent = FVector::CrossProduct(Tangent, Direction).GetSafeNormal();

	TSet<FIntVector> AddedPoints;

	// Scale to make sure we don't miss any point when rounding
	const float Scale = Precision * World->GetVoxelSize();

	for (int X = -Radius; X <= Radius; X += Scale)
	{
		for (int Y = -Radius; Y <= Radius; Y += Scale)
		{
			const float Distance = FVector2D(X, Y).Size();
			if (Distance < Radius)
			{
				FHitResult Hit;
				// Use 2D basis
				FVector Start = ToolPosition + (Tangent * X + Bitangent * Y);
				FVector End = Start + Direction * MaxDistance;
				if (bShowRaycasts)
				{
					DrawDebugLine(World->GetWorld(), Start, End, FColor::Magenta, false, 1);
				}
				if (World->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECollisionChannel::ECC_WorldDynamic) && Hit.Actor->IsA(AVoxelWorld::StaticClass()))
				{
					if (bShowHitPoints)
					{
						DrawDebugPoint(World->GetWorld(), Hit.ImpactPoint, 2, FColor::Red, false, 1);
					}
					for (auto Point : World->GetNeighboringPositions(Hit.ImpactPoint))
					{
						if (bShowModifiedVoxels)
						{
							DrawDebugPoint(World->GetWorld(), World->LocalToGlobal(Point), 3, FColor::White, false, 1);
						}
						if (!AddedPoints.Contains(Point) && LIKELY(World->IsInWorld(Point)))
						{
							AddedPoints.Add(Point);
							OutModifiedPositionsAndDistances.push_front(TTuple<FIntVector, float>(Point, Distance));
						}
					}
				}
			}
		}
	}
}

void UVoxelTools::SetValueProjection(AVoxelWorld* World, const FVector StartPosition, const FVector Direction, const float Radius, const float Strength, const bool bAdd,
	const float MaxDistance, const float Precision, const bool bAsync, const bool bShowRaycasts, const bool bShowHitPoints, const bool bShowModifiedVoxels, const float MinValue, const float MaxValue)
{
	SCOPE_CYCLE_COUNTER(STAT_SetValueProjection);

	if (!World)
	{
		UE_LOG(LogVoxel, Error, TEXT("SetValueProjection: World is NULL"));
		return;
	}

	std::deque<TTuple<FIntVector, float>> ModifiedPositionsAndDistances;
	FindModifiedPositionsForRaycasts(World, StartPosition, Direction, Radius, MaxDistance, Precision, bShowRaycasts, bShowHitPoints, bShowModifiedVoxels, ModifiedPositionsAndDistances);

	for (auto Tuple : ModifiedPositionsAndDistances)
	{
		const FIntVector Point = Tuple.Get<0>();
		const float Distance = Tuple.Get<1>();
		if (World->IsInWorld(Point))
		{
			if (bAdd)
			{
				World->SetValue(Point, FMath::Clamp(World->GetValue(Point) - Strength, MinValue, MaxValue));
			}
			else
			{
				World->SetValue(Point, FMath::Clamp(World->GetValue(Point) + Strength, MinValue, MaxValue));
			}
			World->UpdateChunksAtPosition(Point, bAsync);
		}
	}
}

void UVoxelTools::SetMaterialProjection(AVoxelWorld * World, const FVector StartPosition, const FVector Direction, const float Radius, const uint8 MaterialIndex, const bool bUseLayer1,
	const float FadeDistance, const float MaxDistance, const float Precision, const bool bAsync, const bool bShowRaycasts, const bool bShowHitPoints, const bool bShowModifiedVoxels)
{
	SCOPE_CYCLE_COUNTER(STAT_SetMaterialProjection);

	if (!World)
	{
		UE_LOG(LogVoxel, Error, TEXT("SetMaterialProjection: World is NULL"));
		return;
	}

	const float VoxelDiagonalLength = 1.73205080757f * World->GetVoxelSize();

	std::deque<TTuple<FIntVector, float>> ModifiedPositionsAndDistances;
	FindModifiedPositionsForRaycasts(World, StartPosition, Direction, Radius + FadeDistance + 2 * VoxelDiagonalLength, MaxDistance, Precision, bShowRaycasts, bShowHitPoints, bShowModifiedVoxels, ModifiedPositionsAndDistances);

	for (auto Tuple : ModifiedPositionsAndDistances)
	{
		const FIntVector Point = Tuple.Get<0>();
		const float Distance = Tuple.Get<1>();

		FVoxelMaterial Material = World->GetMaterial(Point);

		if (Distance < Radius + FadeDistance + VoxelDiagonalLength)
		{
			// Set alpha
			int8 Alpha = 255 * FMath::Clamp((Radius + FadeDistance - Distance) / FadeDistance, 0.f, 1.f);
			if (bUseLayer1)
			{
				Alpha = 256 - Alpha;
			}
			if ((bUseLayer1 ? Material.Index1 : Material.Index2) == MaterialIndex)
			{
				// Same color
				Alpha = bUseLayer1 ? FMath::Min<uint8>(Alpha, Material.Alpha) : FMath::Max<uint8>(Alpha, Material.Alpha);
			}
			Material.Alpha = Alpha;

			// Set index
			if (bUseLayer1)
			{
				Material.Index1 = MaterialIndex;
			}
			else
			{
				Material.Index2 = MaterialIndex;
			}

			if (World->IsInWorld(Point))
			{
				// Apply changes
				World->SetMaterial(Point, Material);
				World->UpdateChunksAtPosition(Point, bAsync);
			}
		}
		else if ((bUseLayer1 ? Material.Index1 : Material.Index2) != MaterialIndex)
		{
			Material.Alpha = bUseLayer1 ? 255 : 0;
			if (World->IsInWorld(Point))
			{
				World->SetMaterial(Point, Material);
				World->UpdateChunksAtPosition(Point, bAsync);
			}
		}
	}
}

void UVoxelTools::SmoothValue(AVoxelWorld * World, FVector StartPosition, FVector Direction, float Radius, float Speed, float MaxDistance,
	bool bAsync, bool bDebugLines, bool bDebugPoints, float MinValue, float MaxValue)
{
	SCOPE_CYCLE_COUNTER(STAT_SmoothValue);

	if (World == nullptr)
	{
		UE_LOG(LogVoxel, Error, TEXT("World is NULL"));
		return;
	}
	check(World);
	FVector ToolPosition = StartPosition;

	/**
	* Create a 2D basis from (Tangent, Bitangent)
	*/
	// Compute tangent
	FVector Tangent;
	// N dot T = 0
	// <=> N.X * T.X + N.Y * T.Y + N.Z * T.Z = 0
	// <=> T.Z = -1 / N.Z * (N.X * T.X + N.Y * T.Y) if N.Z != 0
	if (Direction.Z != 0)
	{
		Tangent.X = 1;
		Tangent.Y = 1;
		Tangent.Z = -1 / Direction.Z * (Direction.X * Tangent.X + Direction.Y * Tangent.Y);
	}
	else
	{
		Tangent = FVector(1, 0, 0);
	}

	// Compute bitangent
	FVector Bitangent = FVector::CrossProduct(Tangent, Direction).GetSafeNormal();

	TArray<FIntVector> ModifiedPositions;
	TArray<float> DistancesToTool;

	// Scale to make sure we don't miss any point when rounding
	float Scale = World->GetVoxelSize() / 4;

	int IntRadius = FMath::CeilToInt(Radius);
	for (int x = -IntRadius; x <= IntRadius; x++)
	{
		for (int y = -IntRadius; y <= IntRadius; y++)
		{
			if (x*x + y*y < Radius*Radius)
			{
				FHitResult Hit;
				// Use precedent basis
				FVector Start = ToolPosition + (Tangent * x + Bitangent * y) * Scale;
				FVector End = Start + Direction * MaxDistance;
				if (bDebugLines)
				{
					DrawDebugLine(World->GetWorld(), Start, End, FColor::Magenta, false, 1);
				}
				if (World->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECollisionChannel::ECC_WorldDynamic) && Hit.Actor->IsA(AVoxelWorld::StaticClass()))
				{
					if (bDebugPoints)
					{
						DrawDebugPoint(World->GetWorld(), Hit.ImpactPoint, 2, FColor::Red, false, 1);
					}
					FIntVector ModifiedPosition = World->GlobalToLocal(Hit.ImpactPoint);
					if (!ModifiedPositions.Contains(ModifiedPosition))
					{
						ModifiedPositions.Add(ModifiedPosition);
						DistancesToTool.Add((Hit.ImpactPoint - Start).Size());
					}
				}
			}
		}
	}

	// Compute mean distance
	float MeanDistance(0);
	for (float Distance : DistancesToTool)
	{
		MeanDistance += Distance;
	}
	MeanDistance /= DistancesToTool.Num();

	// Debug
	if (bDebugPoints)
	{
		for (int x = -IntRadius; x <= IntRadius; x++)
		{
			for (int y = -IntRadius; y <= IntRadius; y++)
			{
				if (x*x + y*y < Radius*Radius)
				{
					FHitResult Hit;
					// Use precedent basis
					FVector Start = ToolPosition + (Tangent * x + Bitangent * y) * Scale;
					FVector End = Start - Direction * MaxDistance;

					DrawDebugPoint(World->GetWorld(), Start + (End - Start).GetSafeNormal() * MeanDistance, 2, FColor::Blue, false, 1);
				}
			}
		}
	}

	// Update values
	for (int i = 0; i < ModifiedPositions.Num(); i++)
	{
		FIntVector Point = ModifiedPositions[i];
		float Distance = DistancesToTool[i];
		float Delta = Speed * (MeanDistance - Distance);
		World->SetValue(Point, FMath::Clamp(Delta + World->GetValue(Point), MinValue, MaxValue));
		World->UpdateChunksAtPosition(Point, bAsync);
	}
}

void UVoxelTools::GetVoxelWorld(FVector WorldPosition, FVector WorldDirection, float MaxDistance, APlayerController* PlayerController, AVoxelWorld*& World, FVector& HitPosition, FVector& HitNormal, EBlueprintSuccess& Branches)
{
	if (!PlayerController)
	{
		UE_LOG(LogVoxel, Error, TEXT("GetVoxelWorld: Invalid PlayerController"));
		Branches = EBlueprintSuccess::Failed;
		return;
	}

	FHitResult HitResult;
	if (PlayerController->GetWorld()->LineTraceSingleByChannel(HitResult, WorldPosition, WorldPosition + WorldDirection * MaxDistance, ECC_WorldDynamic))
	{
		if (HitResult.Actor->IsA(AVoxelWorld::StaticClass()))
		{
			World = Cast<AVoxelWorld>(HitResult.Actor.Get());
			check(World);

			HitPosition = HitResult.ImpactPoint;
			HitNormal = HitResult.ImpactNormal;
			Branches = EBlueprintSuccess::Success;
		}
		else
		{
			Branches = EBlueprintSuccess::Failed;
		}
	}
	else
	{
		Branches = EBlueprintSuccess::Failed;
	}
}

void UVoxelTools::GetMouseWorldPositionAndDirection(APlayerController* PlayerController, FVector& WorldPosition, FVector& WorldDirection, EBlueprintSuccess& Branches)
{
	if (!PlayerController)
	{
		UE_LOG(LogVoxel, Error, TEXT("GetMouseWorldPositionAndDirection: Invalid PlayerController"));
		Branches = EBlueprintSuccess::Failed;
		return;
	}
	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(PlayerController->Player);

	if (PlayerController->GetLocalPlayer() && PlayerController->GetLocalPlayer()->ViewportClient)
	{
		FVector2D MousePosition;
		if (PlayerController->GetLocalPlayer()->ViewportClient->GetMousePosition(MousePosition))
		{
			// Early out if we clicked on a HUD hitbox
			if (PlayerController->GetHUD() != NULL && PlayerController->GetHUD()->GetHitBoxAtCoordinates(MousePosition, true))
			{
				Branches = EBlueprintSuccess::Failed;
			}
			else
			{
				if (UGameplayStatics::DeprojectScreenToWorld(PlayerController, MousePosition, WorldPosition, WorldDirection) == true)
				{
					Branches = EBlueprintSuccess::Success;
				}
				else
				{
					Branches = EBlueprintSuccess::Failed;
				}
			}
		}
		else
		{
			Branches = EBlueprintSuccess::Failed;
		}
	}
	else
	{
		Branches = EBlueprintSuccess::Failed;
	}
}