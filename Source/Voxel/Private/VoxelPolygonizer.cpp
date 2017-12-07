// Copyright 2017 Phyronnaz

#include "VoxelPolygonizer.h"
#include "Transvoxel.h"
#include "VoxelData.h"
#include "VoxelMaterial.h"
#include <deque>
#include "Kismet/KismetArrayLibrary.h"

DECLARE_CYCLE_STAT(TEXT("VoxelPolygonizer ~ Cache"), STAT_CACHE, STATGROUP_Voxel);
DECLARE_CYCLE_STAT(TEXT("VoxelPolygonizer ~ Main Iter"), STAT_MAIN_ITER, STATGROUP_Voxel);
DECLARE_CYCLE_STAT(TEXT("VoxelPolygonizer ~ Transitions Iter"), STAT_TRANSITIONS_ITER, STATGROUP_Voxel);
DECLARE_CYCLE_STAT(TEXT("VoxelPolygonizer ~ CreateSection"), STAT_CREATE_SECTION, STATGROUP_Voxel);
DECLARE_CYCLE_STAT(TEXT("VoxelPolygonizer ~ Add transitions to Section"), STAT_ADD_TRANSITIONS_TO_SECTION, STATGROUP_Voxel);
DECLARE_CYCLE_STAT(TEXT("VoxelPolygonizer ~ MajorColor"), STAT_MAJOR_COLOR, STATGROUP_Voxel);
DECLARE_CYCLE_STAT(TEXT("VoxelPolygonizer ~ GetValueAndColor"), STAT_GETVALUEANDCOLOR, STATGROUP_Voxel);
DECLARE_CYCLE_STAT(TEXT("VoxelPolygonizer ~ Get2DValueAndColor"), STAT_GET2DVALUEANDCOLOR, STATGROUP_Voxel);
DECLARE_CYCLE_STAT(TEXT("VoxelPolygonizer ~ AmbientOcclusion"), STAT_AMBIENT_OCCLUSION, STATGROUP_Voxel);

FVoxelPolygonizer::FVoxelPolygonizer(int Depth, FVoxelData* Data, const FIntVector& ChunkPosition, const TArray<bool, TFixedAllocator<6>>& ChunkHasHigherRes, bool bComputeTransitions, bool bComputeCollisions, bool bEnableAmbientOcclusion, int RayMaxDistance, int RayCount, float NormalThresholdForSimplification)
	: Depth(Depth)
	, Data(Data)
	, ChunkPosition(ChunkPosition)
	, ChunkHasHigherRes(ChunkHasHigherRes)
	, bComputeTransitions(bComputeTransitions)
	, bComputeCollisions(bComputeCollisions)
	, bEnableAmbientOcclusion(bEnableAmbientOcclusion)
	, RayMaxDistance(RayMaxDistance)
	, RayCount(RayCount)
	, NormalThresholdForSimplification(NormalThresholdForSimplification)
{

}

void FVoxelPolygonizer::CreateSection(FVoxelProcMeshSection& OutSection)
{
	for (int i = 0; i < 17; i++)
	{
		for (int j = 0; j < 17; j++)
		{
			for (int k = 0; k < 17; k++)
			{
				IntegerCoordinates[i][j][k] = -1;
			}
		}
	}

	{
		SCOPE_CYCLE_COUNTER(STAT_CACHE);

		FIntVector Size(CHUNKSIZE + 3, CHUNKSIZE + 3, CHUNKSIZE + 3);
		Data->BeginGet();
		Data->GetValuesAndMaterials(CachedValues, CachedMaterials, ChunkPosition - FIntVector(1, 1, 1) * Step(), FIntVector::ZeroValue, Step(), Size, Size);
		Data->EndGet();

		// Cache signs
		for (int CubeX = 0; CubeX < 6; CubeX++)
		{
			for (int CubeY = 0; CubeY < 6; CubeY++)
			{
				for (int CubeZ = 0; CubeZ < 6; CubeZ++)
				{
					uint64& CurrentCube = CachedSigns[CubeX + 6 * CubeY + 6 * 6 * CubeZ];
					CurrentCube = 0;
					for (int LocalX = 0; LocalX < 4; LocalX++)
					{
						for (int LocalY = 0; LocalY < 4; LocalY++)
						{
							for (int LocalZ = 0; LocalZ < 4; LocalZ++)
							{
								// -1: offset because of normals computations
								const int X = 3 * CubeX + LocalX - 1;
								const int Y = 3 * CubeY + LocalY - 1;
								const int Z = 3 * CubeZ + LocalZ - 1;

								const uint64 ONE = 1;
								uint64 CurrentBit = ONE << (LocalX + 4 * LocalY + 4 * 4 * LocalZ);


								check(0 <= X + 1 && X + 1 < CHUNKSIZE + 3);
								check(0 <= Y + 1 && Y + 1 < CHUNKSIZE + 3);
								check(0 <= Z + 1 && Z + 1 < CHUNKSIZE + 3);
								float CurrentValue = CachedValues[(X + 1) + (CHUNKSIZE + 3) * (Y + 1) + (CHUNKSIZE + 3) * (CHUNKSIZE + 3) * (Z + 1)];

								bool Sign = CurrentValue > 0;
								CurrentCube = CurrentCube | (CurrentBit * Sign);
							}
						}
					}
				}
			}
		}
	}

	// Create forward lists
	std::deque<FVector> Vertices;
	std::deque<FColor> Colors;
	std::deque<int32> Triangles;
	int VerticesSize = 0;
	int TrianglesSize = 0;

	{
		SCOPE_CYCLE_COUNTER(STAT_MAIN_ITER);
		// Iterate over cubes
		for (int CubeX = 0; CubeX < 6; CubeX++)
		{
			for (int CubeY = 0; CubeY < 6; CubeY++)
			{
				for (int CubeZ = 0; CubeZ < 6; CubeZ++)
				{
					uint64 CurrentCube = CachedSigns[CubeX + 6 * CubeY + 6 * 6 * CubeZ];
					if (CurrentCube == 0 || CurrentCube == /*MAXUINT64*/ ((uint64)~((uint64)0)))
					{
						continue;
					}
					Data->BeginGet();
					for (int LocalX = 0; LocalX < 3; LocalX++)
					{
						for (int LocalY = 0; LocalY < 3; LocalY++)
						{
							for (int LocalZ = 0; LocalZ < 3; LocalZ++)
							{
								const uint64 ONE = 1;
								unsigned long CaseCode =
									(static_cast<bool>((CurrentCube & (ONE << ((LocalX + 0) + 4 * (LocalY + 0) + 4 * 4 * (LocalZ + 0)))) != 0) << 0)
									| (static_cast<bool>((CurrentCube & (ONE << ((LocalX + 1) + 4 * (LocalY + 0) + 4 * 4 * (LocalZ + 0)))) != 0) << 1)
									| (static_cast<bool>((CurrentCube & (ONE << ((LocalX + 0) + 4 * (LocalY + 1) + 4 * 4 * (LocalZ + 0)))) != 0) << 2)
									| (static_cast<bool>((CurrentCube & (ONE << ((LocalX + 1) + 4 * (LocalY + 1) + 4 * 4 * (LocalZ + 0)))) != 0) << 3)
									| (static_cast<bool>((CurrentCube & (ONE << ((LocalX + 0) + 4 * (LocalY + 0) + 4 * 4 * (LocalZ + 1)))) != 0) << 4)
									| (static_cast<bool>((CurrentCube & (ONE << ((LocalX + 1) + 4 * (LocalY + 0) + 4 * 4 * (LocalZ + 1)))) != 0) << 5)
									| (static_cast<bool>((CurrentCube & (ONE << ((LocalX + 0) + 4 * (LocalY + 1) + 4 * 4 * (LocalZ + 1)))) != 0) << 6)
									| (static_cast<bool>((CurrentCube & (ONE << ((LocalX + 1) + 4 * (LocalY + 1) + 4 * 4 * (LocalZ + 1)))) != 0) << 7);

								if (CaseCode != 0 && CaseCode != 511)
								{
									// Cell has a nontrivial triangulation

									// -1: offset because of normals computations
									const int X = 3 * CubeX + LocalX - 1;
									const int Y = 3 * CubeY + LocalY - 1;
									const int Z = 3 * CubeZ + LocalZ - 1;

									short ValidityMask = (X != -1) + 2 * (Y != -1) + 4 * (Z != -1);

									const FIntVector CornerPositions[8] = {
										FIntVector(X + 0, Y + 0, Z + 0) * Step(),
										FIntVector(X + 1, Y + 0, Z + 0) * Step(),
										FIntVector(X + 0, Y + 1, Z + 0) * Step(),
										FIntVector(X + 1, Y + 1, Z + 0) * Step(),
										FIntVector(X + 0, Y + 0, Z + 1) * Step(),
										FIntVector(X + 1, Y + 0, Z + 1) * Step(),
										FIntVector(X + 0, Y + 1, Z + 1) * Step(),
										FIntVector(X + 1, Y + 1, Z + 1) * Step()
									};

									float CornerValues[8];

									FVoxelMaterial CornerMaterials[8];

									GetValueAndMaterialFromCache(X + 0, Y + 0, Z + 0, CornerValues[0], CornerMaterials[0]);
									GetValueAndMaterialFromCache(X + 1, Y + 0, Z + 0, CornerValues[1], CornerMaterials[1]);
									GetValueAndMaterialFromCache(X + 0, Y + 1, Z + 0, CornerValues[2], CornerMaterials[2]);
									GetValueAndMaterialFromCache(X + 1, Y + 1, Z + 0, CornerValues[3], CornerMaterials[3]);
									GetValueAndMaterialFromCache(X + 0, Y + 0, Z + 1, CornerValues[4], CornerMaterials[4]);
									GetValueAndMaterialFromCache(X + 1, Y + 0, Z + 1, CornerValues[5], CornerMaterials[5]);
									GetValueAndMaterialFromCache(X + 0, Y + 1, Z + 1, CornerValues[6], CornerMaterials[6]);
									GetValueAndMaterialFromCache(X + 1, Y + 1, Z + 1, CornerValues[7], CornerMaterials[7]);

									check(CaseCode == (
										((CornerValues[0] > 0) << 0)
										| ((CornerValues[1] > 0) << 1)
										| ((CornerValues[2] > 0) << 2)
										| ((CornerValues[3] > 0) << 3)
										| ((CornerValues[4] > 0) << 4)
										| ((CornerValues[5] > 0) << 5)
										| ((CornerValues[6] > 0) << 6)
										| ((CornerValues[7] > 0) << 7)));

									check(0 <= CaseCode && CaseCode < 256);
									unsigned char CellClass = Transvoxel::regularCellClass[CaseCode];
									const unsigned short* VertexData = Transvoxel::regularVertexData[CaseCode];
									check(0 <= CellClass && CellClass < 16);
									Transvoxel::RegularCellData CellData = Transvoxel::regularCellData[CellClass];

									// Indices of the vertices used in this cube
									TArray<int> VertexIndices;
									VertexIndices.SetNumUninitialized(CellData.GetVertexCount());

									for (int i = 0; i < CellData.GetVertexCount(); i++)
									{
										int VertexIndex;
										const unsigned short EdgeCode = VertexData[i];

										// A: low point / B: high point
										const unsigned short IndexVerticeA = (EdgeCode >> 4) & 0x0F;
										const unsigned short IndexVerticeB = EdgeCode & 0x0F;

										check(0 <= IndexVerticeA && IndexVerticeA < 8);
										check(0 <= IndexVerticeB && IndexVerticeB < 8);

										const FIntVector PositionA = CornerPositions[IndexVerticeA];
										const FIntVector PositionB = CornerPositions[IndexVerticeB];

										// Index of vertex on a generic cube (0, 1, 2 or 3 in the transvoxel paper, but it's always != 0 so we substract 1 to have 0, 1, or 2)
										const short EdgeIndex = ((EdgeCode >> 8) & 0x0F) - 1;
										check(0 <= EdgeIndex && EdgeIndex < 3);

										// Direction to go to use an already created vertex: 
										// first bit:  x is different
										// second bit: y is different
										// third bit:  z is different
										// fourth bit: vertex isn't cached
										const short CacheDirection = EdgeCode >> 12;


										if ((ValidityMask & CacheDirection) != CacheDirection)
										{
											// If we are on one the lower edges of the chunk, or precedent color is not the same as current one

											const bool bIsAlongX = (EdgeIndex == 1);
											const bool bIsAlongY = (EdgeIndex == 0);
											const bool bIsAlongZ = (EdgeIndex == 2);

											FVector Q;
											uint8 Alpha;
											if (Step() == 1)
											{
												// Full resolution

												const float ValueAtA = CornerValues[IndexVerticeA];
												const float ValueAtB = CornerValues[IndexVerticeB];

												const float	AlphaAtA = CornerMaterials[IndexVerticeA].Alpha;
												const float AlphaAtB = CornerMaterials[IndexVerticeB].Alpha;

												check(ValueAtA - ValueAtB != 0);
												const float t = ValueAtB / (ValueAtB - ValueAtA);

												Q = t * static_cast<FVector>(PositionA) + (1 - t) * static_cast<FVector>(PositionB);
												Alpha = t * AlphaAtA + (1 - t) * AlphaAtB;
											}
											else
											{
												// Interpolate

												if (bIsAlongX)
												{
													InterpolateX(PositionA.X, PositionB.X, PositionA.Y, PositionA.Z, Q, Alpha);
												}
												else if (bIsAlongY)
												{
													InterpolateY(PositionA.X, PositionA.Y, PositionB.Y, PositionA.Z, Q, Alpha);
												}
												else if (bIsAlongZ)
												{
													InterpolateZ(PositionA.X, PositionA.Y, PositionA.Z, PositionB.Z, Q, Alpha);
												}
												else
												{
													Alpha = 0;
													checkf(false, TEXT("Error in interpolation: case should not exist"));
												}
											}

											VertexIndex = VerticesSize;

											bool bCreateVertex = true;

											if (!(
												(Q.X < -KINDA_SMALL_NUMBER) || (Q.X > Size() + KINDA_SMALL_NUMBER) ||
												(Q.Y < -KINDA_SMALL_NUMBER) || (Q.Y > Size() + KINDA_SMALL_NUMBER) ||
												(Q.Z < -KINDA_SMALL_NUMBER) || (Q.Z > Size() + KINDA_SMALL_NUMBER)))
											{
												// Not for normal only

												if (FMath::Abs(Q.X - FMath::RoundToInt(Q.X)) < KINDA_SMALL_NUMBER &&
													FMath::Abs(Q.Y - FMath::RoundToInt(Q.Y)) < KINDA_SMALL_NUMBER &&
													FMath::Abs(Q.Z - FMath::RoundToInt(Q.Z)) < KINDA_SMALL_NUMBER)
												{
													Q.X = FMath::RoundToInt(Q.X);
													Q.Y = FMath::RoundToInt(Q.Y);
													Q.Z = FMath::RoundToInt(Q.Z);

													const int IX = FMath::RoundToInt(Q.X / Step());
													const int IY = FMath::RoundToInt(Q.Y / Step());
													const int IZ = FMath::RoundToInt(Q.Z / Step());

													check(0 <= IX && IX < 17);
													check(0 <= IY && IY < 17);
													check(0 <= IZ && IZ < 17);

													if (IntegerCoordinates[IX][IY][IZ] == -1)
													{
														IntegerCoordinates[IX][IY][IZ] = VertexIndex;
													}
													else
													{
														VertexIndex = IntegerCoordinates[IX][IY][IZ];
														bCreateVertex = false;
													}
												}
											}

											if (bCreateVertex)
											{
												Vertices.push_front(Q);
												FVoxelMaterial Material = (CornerValues[IndexVerticeA] <= 0) ? CornerMaterials[IndexVerticeA] : CornerMaterials[IndexVerticeB];
												Colors.push_front(FVoxelMaterial(Material.Index1, Material.Index2, Alpha).ToFColor());
												VerticesSize++;
											}
										}
										else
										{
											VertexIndex = LoadVertex(X, Y, Z, CacheDirection, EdgeIndex);
										}

										// If own vertex, save it
										if (CacheDirection & 0x08)
										{
											SaveVertex(X, Y, Z, EdgeIndex, VertexIndex);
										}

										VertexIndices[i] = VertexIndex;
									}

									// Add triangles
									// 3 vertex per triangle
									int n = 3 * CellData.GetTriangleCount();
									for (int i = 0; i < n; i++)
									{
										Triangles.push_front(VertexIndices[CellData.vertexIndex[i]]);
									}
									TrianglesSize += n;
								}
							}
						}
					}
					Data->EndGet();
				}
			}
		}
	}


	if (VerticesSize < 3)
	{
		// Early exit
		OutSection.Reset();
		return;
	}

	TArray<TArray<int32>> VerticesTriangles;
	TArray<int32> AllToFiltered;
	int32 FilteredVertexCount;
	int32 FilteredTriangleCount;
	{
		SCOPE_CYCLE_COUNTER(STAT_CREATE_SECTION);
		// Create section
		OutSection.Reset();
		OutSection.bEnableCollision = bComputeCollisions;
		OutSection.bSectionVisible = true;
		OutSection.SectionLocalBox.Min = -FVector::OneVector * Step();
		OutSection.SectionLocalBox.Max = 18 * FVector::OneVector * Step();
		OutSection.SectionLocalBox.IsValid = true;

		OutSection.ProcVertexBuffer.SetNumUninitialized(VerticesSize);
		OutSection.ProcIndexBuffer.SetNumUninitialized(TrianglesSize);

		// We create 2 vertex arrays: one with all the vertex (AllVertex), and one without vertex that are only for normal computation (FilteredVertex) which is the Section array
		// To do so we create bijection between the 2 arrays (AllToFiltered)
		AllToFiltered.SetNumUninitialized(VerticesSize);

		TArray<FVector> AllVertex;
		AllVertex.SetNumUninitialized(VerticesSize);

		int32 AllVertexIndex = 0;
		int32 FilteredVertexIndex = 0;
		auto VerticesIt = Vertices.begin();
		auto ColorsIt = Colors.begin();
		for (int i = VerticesSize - 1; i >= 0; i--)
		{
			FVector Vertex = *VerticesIt;
			FColor Color = *ColorsIt;
			++VerticesIt;
			++ColorsIt;

			if ((Vertex.X < -KINDA_SMALL_NUMBER) || (Vertex.X > Size() + KINDA_SMALL_NUMBER) ||
				(Vertex.Y < -KINDA_SMALL_NUMBER) || (Vertex.Y > Size() + KINDA_SMALL_NUMBER) ||
				(Vertex.Z < -KINDA_SMALL_NUMBER) || (Vertex.Z > Size() + KINDA_SMALL_NUMBER))
			{
				// For normals only
				AllToFiltered[AllVertexIndex] = -1;
			}
			else
			{
				AllToFiltered[AllVertexIndex] = FilteredVertexIndex;

				FVoxelProcMeshVertex& ProcMeshVertex = OutSection.ProcVertexBuffer[FilteredVertexIndex];
				ProcMeshVertex.Position = Vertex;
				ProcMeshVertex.Tangent = FVoxelProcMeshTangent();
				ProcMeshVertex.Color = Color;
				ProcMeshVertex.UV0 = FVector2D::ZeroVector;
				//OutSection.SectionLocalBox += Vertex;

				FilteredVertexIndex++;
			}

			AllVertex[AllVertexIndex] = Vertex;
			AllVertexIndex++;
		}
		FilteredVertexCount = FilteredVertexIndex;
		OutSection.ProcVertexBuffer.SetNum(FilteredVertexCount);

		// Normal array to compute normals while iterating over triangles
		TArray<FVector> Normals;
		Normals.SetNumZeroed(FilteredVertexCount); // Zeroed because +=

		VerticesTriangles.SetNum(FilteredVertexCount);

		int32 FilteredTriangleIndex = 0;
		for (auto TrianglesIt = Triangles.begin(); TrianglesIt != Triangles.end(); )
		{
			int32 A = *TrianglesIt;
			++TrianglesIt;

			int32 B = *TrianglesIt;
			++TrianglesIt;

			int32 C = *TrianglesIt;
			++TrianglesIt;

			{
				// Add trig

				int32 FA = AllToFiltered[VerticesSize - 1 - A]; // Invert triangles because AllVertex and OutSection.ProcVertexBuffer are inverted
				int32 FB = AllToFiltered[VerticesSize - 1 - B];
				int32 FC = AllToFiltered[VerticesSize - 1 - C];

				if (FA != -1 && FB != -1 && FC != -1)
				{
					// If all vertex of this triangle are not for normal only, this is a valid triangle
					OutSection.ProcIndexBuffer[FilteredTriangleIndex] = FC;
					OutSection.ProcIndexBuffer[FilteredTriangleIndex + 1] = FB;
					OutSection.ProcIndexBuffer[FilteredTriangleIndex + 2] = FA;

					VerticesTriangles[FA].AddUnique(FilteredTriangleIndex);
					VerticesTriangles[FB].AddUnique(FilteredTriangleIndex);
					VerticesTriangles[FC].AddUnique(FilteredTriangleIndex);

					FilteredTriangleIndex += 3;
				}
			}

			{
				// Compute normals

				FVector PA = AllVertex[VerticesSize - 1 - A];
				FVector PB = AllVertex[VerticesSize - 1 - B];
				FVector PC = AllVertex[VerticesSize - 1 - C];

				int32 FA = AllToFiltered[VerticesSize - 1 - A];
				int32 FB = AllToFiltered[VerticesSize - 1 - B];
				int32 FC = AllToFiltered[VerticesSize - 1 - C];

				FVector Normal = FVector::CrossProduct(PB - PA, PC - PA).GetSafeNormal();
				if (FA != -1)
				{
					Normals[FA] += Normal;
				}
				if (FB != -1)
				{
					Normals[FB] += Normal;
				}
				if (FC != -1)
				{
					Normals[FC] += Normal;
				}
			}
		}
		FilteredTriangleCount = FilteredTriangleIndex;;
		OutSection.ProcIndexBuffer.SetNum(FilteredTriangleCount);

		// Apply normals
		for (int32 i = 0; i < FilteredVertexCount; i++)
		{
			FVoxelProcMeshVertex& ProcMeshVertex = OutSection.ProcVertexBuffer[i];
			ProcMeshVertex.Normal = Normals[i].GetSafeNormal();

			ProcMeshVertex.Position = bComputeTransitions ? GetTranslated(ProcMeshVertex.Position, ProcMeshVertex.Normal) : ProcMeshVertex.Position;
		}
	}

	Vertices.clear();
	Triangles.clear();

	check(Vertices.empty());
	check(Triangles.empty());

	/*
	{
		TArray<TPair<int, int>> VerticesToCollapse;

		for (int U = 0; U < OutSection.ProcVertexBuffer.Num(); U++)
		{
			TSet<int32> Neighbors;
			for (int32 Trig : VerticesTriangles[U])
			{
				check(Trig % 3 == 0);

				int VA = OutSection.ProcIndexBuffer[Trig];
				int VB = OutSection.ProcIndexBuffer[Trig + 1];
				int VC = OutSection.ProcIndexBuffer[Trig + 2];

				if (VA < U)
				{
					Neighbors.Add(VA);
				}
				if (VB < U)
				{
					Neighbors.Add(VB);
				}
				if (VC < U)
				{
					Neighbors.Add(VC);
				}
			}

			int MinV = -1;
			int MinCurvature = 1;
			for (int V : Neighbors)
			{
				check(V < U);
				FVector PosU = OutSection.ProcVertexBuffer[U].Position;
				FVector PosV = OutSection.ProcVertexBuffer[V].Position;

				if (PosU.GetMax() >= Size() - Step() - KINDA_SMALL_NUMBER || PosU.GetMin() <= Step() + KINDA_SMALL_NUMBER)
				{
					continue;
				}

				TArray<int32> Sides;
				for (int32 Trig : VerticesTriangles[U])
				{
					check(Trig % 3 == 0);

					if (OutSection.ProcIndexBuffer[Trig] == V ||
						OutSection.ProcIndexBuffer[Trig + 1] == V ||
						OutSection.ProcIndexBuffer[Trig + 2] == V)
					{
						Sides.Add(Trig);
					}
				}
				// use the triangle facing most away from the sides
				// to determine our curvature term
				float Curvature = 0;
				for (int32 Face : VerticesTriangles[U])
				{
					check(Face % 3 == 0);

					float MinCurv = 1;
					for (uint32 Side : Sides)
					{
						FVector FaceNormal = FVector::ZeroVector;
						FaceNormal += OutSection.ProcVertexBuffer[OutSection.ProcIndexBuffer[Face]].Normal;
						FaceNormal += OutSection.ProcVertexBuffer[OutSection.ProcIndexBuffer[Face + 1]].Normal;
						FaceNormal += OutSection.ProcVertexBuffer[OutSection.ProcIndexBuffer[Face + 2]].Normal;
						FaceNormal.Normalize();

						FVector SideNormal = FVector::ZeroVector;
						SideNormal += OutSection.ProcVertexBuffer[OutSection.ProcIndexBuffer[Side]].Normal;
						SideNormal += OutSection.ProcVertexBuffer[OutSection.ProcIndexBuffer[Side + 1]].Normal;
						SideNormal += OutSection.ProcVertexBuffer[OutSection.ProcIndexBuffer[Side + 2]].Normal;
						SideNormal.Normalize();

						MinCurv = FMath::Min(MinCurv, (1 - FVector::DotProduct(FaceNormal, SideNormal)) / 2.0f);
					}
					Curvature = FMath::Max(Curvature, MinCurv);
				}
				if (Curvature < MinCurvature)
				{
					MinCurvature = Curvature;
					MinV = V;
				}
			}
			if (MinCurvature < NormalThresholdForSimplification && MinV != -1)
			{
				VerticesToCollapse.Add(TPair<int, int>(U, MinV));
			}
		}

		TArray<int> Bijection;
		Bijection.SetNumUninitialized(OutSection.ProcVertexBuffer.Num());
		for (int i = 0; i < Bijection.Num(); i++)
		{
			Bijection[i] = i;
		}

		for (TPair<int, int> Pair : VerticesToCollapse)
		{
			int U = Pair.Get<0>();
			int V = Pair.Get<1>();

			check(V < U);

			while (Bijection[U] != U)
			{
				U = Bijection[U];
			}
			while (Bijection[V] != V)
			{
				V = Bijection[V];
			}
			check(Bijection[U] == U);
			check(Bijection[V] == V);

			if (FVector::Distance(OutSection.ProcVertexBuffer[U].Position, OutSection.ProcVertexBuffer[V].Position) < 2 * Step())
			{
				// delete triangles on edge uv:
				TArray<int32> RemainingSides;
				TArray<int32> Sides;
				for (int32 Trig : VerticesTriangles[U])
				{
					check(Trig % 3 == 0);

					int VA = OutSection.ProcIndexBuffer[Trig];
					int VB = OutSection.ProcIndexBuffer[Trig + 1];
					int VC = OutSection.ProcIndexBuffer[Trig + 2];

					if (VA == V || VB == V || VC == V)
					{
						if (VA != U)
						{
							VerticesTriangles[VA].Remove(Trig);
						}
						if (VB != U)
						{
							VerticesTriangles[VB].Remove(Trig);
						}
						if (VC != U)
						{
							VerticesTriangles[VC].Remove(Trig);
						}
						OutSection.ProcIndexBuffer[Trig] = 0;
						OutSection.ProcIndexBuffer[Trig + 1] = 0;
						OutSection.ProcIndexBuffer[Trig + 2] = 0;
					}
					else
					{
						RemainingSides.AddUnique(Trig);
					}
				}

				// update remaining triangles to have v instead of u
				for (auto Trig : RemainingSides)
				{
					VerticesTriangles[V].AddUnique(Trig);
					if (OutSection.ProcIndexBuffer[Trig] == U)
					{
						OutSection.ProcIndexBuffer[Trig] = V;
					}
					if (OutSection.ProcIndexBuffer[Trig + 1] == U)
					{
						OutSection.ProcIndexBuffer[Trig + 1] = V;
					}
					if (OutSection.ProcIndexBuffer[Trig + 2] == U)
					{
						OutSection.ProcIndexBuffer[Trig + 2] = V;
					}
				}
				VerticesTriangles[U].Empty();
				Bijection[U] = V;

				// Delete U
				OutSection.ProcVertexBuffer[U].Position = FVector::OneVector * 100;
			}
		}
	}
	*/
	if (bComputeTransitions)
	{
		const int OldVerticesSize = VerticesSize;
		const int OldTrianglesSize = TrianglesSize;

		Data->BeginGet();
		{
			SCOPE_CYCLE_COUNTER(STAT_TRANSITIONS_ITER);

			for (int DirectionIndex = 0; DirectionIndex < 6; DirectionIndex++)
			{
				auto Direction = (EDirection)DirectionIndex;

				if (ChunkHasHigherRes[Direction])
				{
					for (int X = 0; X < 16; X++)
					{
						for (int Y = 0; Y < 16; Y++)
						{
							const int HalfStep = Step() / 2;

							float CornerValues[9];
							FVoxelMaterial CornerMaterials[9];

							Get2DValueAndMaterial(Direction, (2 * X + 0) * HalfStep, (2 * Y + 0) * HalfStep, CornerValues[0], CornerMaterials[0]);
							Get2DValueAndMaterial(Direction, (2 * X + 1) * HalfStep, (2 * Y + 0) * HalfStep, CornerValues[1], CornerMaterials[1]);
							Get2DValueAndMaterial(Direction, (2 * X + 2) * HalfStep, (2 * Y + 0) * HalfStep, CornerValues[2], CornerMaterials[2]);
							Get2DValueAndMaterial(Direction, (2 * X + 0) * HalfStep, (2 * Y + 1) * HalfStep, CornerValues[3], CornerMaterials[3]);
							Get2DValueAndMaterial(Direction, (2 * X + 1) * HalfStep, (2 * Y + 1) * HalfStep, CornerValues[4], CornerMaterials[4]);
							Get2DValueAndMaterial(Direction, (2 * X + 2) * HalfStep, (2 * Y + 1) * HalfStep, CornerValues[5], CornerMaterials[5]);
							Get2DValueAndMaterial(Direction, (2 * X + 0) * HalfStep, (2 * Y + 2) * HalfStep, CornerValues[6], CornerMaterials[6]);
							Get2DValueAndMaterial(Direction, (2 * X + 1) * HalfStep, (2 * Y + 2) * HalfStep, CornerValues[7], CornerMaterials[7]);
							Get2DValueAndMaterial(Direction, (2 * X + 2) * HalfStep, (2 * Y + 2) * HalfStep, CornerValues[8], CornerMaterials[8]);

							unsigned long CaseCode =
								(static_cast<bool>(CornerValues[0] > 0) << 0)
								| (static_cast<bool>(CornerValues[1] > 0) << 1)
								| (static_cast<bool>(CornerValues[2] > 0) << 2)
								| (static_cast<bool>(CornerValues[5] > 0) << 3)
								| (static_cast<bool>(CornerValues[8] > 0) << 4)
								| (static_cast<bool>(CornerValues[7] > 0) << 5)
								| (static_cast<bool>(CornerValues[6] > 0) << 6)
								| (static_cast<bool>(CornerValues[3] > 0) << 7)
								| (static_cast<bool>(CornerValues[4] > 0) << 8);

							if (!(CaseCode == 0 || CaseCode == 511))
							{
								short ValidityMask = (X != 0) + 2 * (Y != 0);

								const FVoxelMaterial CellMaterial = CornerMaterials[0];

								FIntVector Positions[13] = {
									FIntVector(2 * X + 0, 2 * Y + 0, 0) * HalfStep,
									FIntVector(2 * X + 1, 2 * Y + 0, 0) * HalfStep,
									FIntVector(2 * X + 2, 2 * Y + 0, 0) * HalfStep,
									FIntVector(2 * X + 0, 2 * Y + 1, 0) * HalfStep,
									FIntVector(2 * X + 1, 2 * Y + 1, 0) * HalfStep,
									FIntVector(2 * X + 2, 2 * Y + 1, 0) * HalfStep,
									FIntVector(2 * X + 0, 2 * Y + 2, 0) * HalfStep,
									FIntVector(2 * X + 1, 2 * Y + 2, 0) * HalfStep,
									FIntVector(2 * X + 2, 2 * Y + 2, 0) * HalfStep,

									FIntVector(2 * X + 0, 2 * Y + 0, 1) * HalfStep,
									FIntVector(2 * X + 2, 2 * Y + 0, 1) * HalfStep,
									FIntVector(2 * X + 0, 2 * Y + 2, 1) * HalfStep,
									FIntVector(2 * X + 2, 2 * Y + 2, 1) * HalfStep
								};

								check(0 <= CaseCode && CaseCode < 512);
								const unsigned char CellClass = Transvoxel::transitionCellClass[CaseCode];
								const unsigned short* VertexData = Transvoxel::transitionVertexData[CaseCode];
								check(0 <= (CellClass & 0x7F) && (CellClass & 0x7F) < 56);
								const Transvoxel::TransitionCellData CellData = Transvoxel::transitionCellData[CellClass & 0x7F];
								const bool bFlip = ((CellClass >> 7) != 0);

								TArray<int> VertexIndices;
								VertexIndices.SetNumUninitialized(CellData.GetVertexCount());

								for (int i = 0; i < CellData.GetVertexCount(); i++)
								{
									int VertexIndex;
									const unsigned short EdgeCode = VertexData[i];

									// A: low point / B: high point
									const unsigned short IndexVerticeA = (EdgeCode >> 4) & 0x0F;
									const unsigned short IndexVerticeB = EdgeCode & 0x0F;

									check(0 <= IndexVerticeA && IndexVerticeA < 13);
									check(0 <= IndexVerticeB && IndexVerticeB < 13);

									const FIntVector PositionA = Positions[IndexVerticeA];
									const FIntVector PositionB = Positions[IndexVerticeB];

									const short EdgeIndex = (EdgeCode >> 8) & 0x0F;
									// Direction to go to use an already created vertex
									const short CacheDirection = EdgeCode >> 12;

									if ((ValidityMask & CacheDirection) != CacheDirection && EdgeIndex != 8 && EdgeIndex != 9)
									{
										// Validity check failed or EdgeIndex == 8 | 9 (always use Cache2D)
										const bool bIsAlongX = EdgeIndex == 3 || EdgeIndex == 4 || EdgeIndex == 8;
										const bool bIsAlongY = EdgeIndex == 5 || EdgeIndex == 6 || EdgeIndex == 9;


										FVector Q;
										uint8 Alpha;

										if (bIsAlongX)
										{
											// Edge along X axis
											InterpolateX2D(Direction, PositionA.X, PositionB.X, PositionA.Y, Q, Alpha);
										}
										else if (bIsAlongY)
										{
											// Edge along Y axis
											InterpolateY2D(Direction, PositionA.X, PositionA.Y, PositionB.Y, Q, Alpha);
										}
										else
										{
											Alpha = 0;
											checkf(false, TEXT("Error in interpolation: case should not exist"));
										}

										VertexIndex = VerticesSize;
										Vertices.push_front(Q);
										Colors.push_front(FVoxelMaterial(CellMaterial.Index1, CellMaterial.Index2, Alpha).ToFColor());
										VerticesSize++;

										// If own vertex, save it
										if (CacheDirection & 0x08)
										{
											SaveVertex2D(Direction, X, Y, EdgeIndex, VertexIndex);
										}
									}
									else
									{
										VertexIndex = LoadVertex2D(Direction, X, Y, CacheDirection, EdgeIndex);
									}

									VertexIndices[i] = VertexIndex;
								}

								// Add triangles
								int n = 3 * CellData.GetTriangleCount();
								for (int i = 0; i < n; i++)
								{
									Triangles.push_front(VertexIndices[CellData.vertexIndex[bFlip ? (n - 1 - i) : i]]);
								}
								TrianglesSize += n;
							}
						}
					}
				}
			}
		}
		Data->EndGet();

		{
			SCOPE_CYCLE_COUNTER(STAT_ADD_TRANSITIONS_TO_SECTION);

			const int32 TransitionsVerticesSize = VerticesSize - OldVerticesSize;
			const int32 TransitionTrianglesSize = TrianglesSize - OldTrianglesSize;

			OutSection.ProcVertexBuffer.AddUninitialized(TransitionsVerticesSize);
			OutSection.ProcIndexBuffer.AddUninitialized(TransitionTrianglesSize);

			TArray<FVector> TransitionVertex;
			TransitionVertex.SetNumUninitialized(TransitionsVerticesSize);

			auto VerticesIt = Vertices.begin();
			auto ColorsIt = Colors.begin();
			for (int TransitionVertexIndex = TransitionsVerticesSize - 1; TransitionVertexIndex >= 0; TransitionVertexIndex--)
			{
				FVector Vertex = *VerticesIt;
				FColor Color = *ColorsIt;
				++VerticesIt;
				++ColorsIt;

				FVoxelProcMeshVertex& ProcMeshVertex = OutSection.ProcVertexBuffer[FilteredVertexCount + TransitionVertexIndex];
				ProcMeshVertex.Position = Vertex;
				ProcMeshVertex.Tangent = FVoxelProcMeshTangent();
				ProcMeshVertex.Color = Color;
				ProcMeshVertex.UV0 = FVector2D::ZeroVector;
				//OutSection.SectionLocalBox += Vertex;

				TransitionVertex[TransitionVertexIndex] = Vertex;
			}

			// Normal array to compute normals while iterating over triangles
			TArray<FVector> Normals;
			Normals.SetNumUninitialized(TransitionsVerticesSize);

			int32 TransitionTriangleIndex = 0;
			for (auto TrianglesIt = Triangles.begin(); TrianglesIt != Triangles.end(); )
			{
				int32 A = *TrianglesIt;
				// OldVerticesSize - FilteredVertexCount because we need to offset index due to vertex removal for normals
				int32 FA = A < OldVerticesSize ? AllToFiltered[OldVerticesSize - 1 - A] : A - (OldVerticesSize - FilteredVertexCount);
				++TrianglesIt;

				int32 B = *TrianglesIt;
				int32 FB = B < OldVerticesSize ? AllToFiltered[OldVerticesSize - 1 - B] : B - (OldVerticesSize - FilteredVertexCount);
				++TrianglesIt;

				int32 C = *TrianglesIt;
				int32 FC = C < OldVerticesSize ? AllToFiltered[OldVerticesSize - 1 - C] : C - (OldVerticesSize - FilteredVertexCount);
				++TrianglesIt;

				check(FA != -1 && FB != -1 && FC != -1);

				OutSection.ProcIndexBuffer[FilteredTriangleCount + TransitionTriangleIndex] = FC;
				OutSection.ProcIndexBuffer[FilteredTriangleCount + TransitionTriangleIndex + 1] = FB;
				OutSection.ProcIndexBuffer[FilteredTriangleCount + TransitionTriangleIndex + 2] = FA;
				TransitionTriangleIndex += 3;

				FVector Normal;
				if (A < OldVerticesSize)
				{
					Normal = OutSection.ProcVertexBuffer[FA].Normal;
				}
				else if (B < OldVerticesSize)
				{
					Normal = OutSection.ProcVertexBuffer[FB].Normal;
				}
				else if (C < OldVerticesSize)
				{
					Normal = OutSection.ProcVertexBuffer[FC].Normal;
				}


				if (A >= OldVerticesSize)
				{
					Normals[FA - FilteredVertexCount] = Normal;
				}
				if (B >= OldVerticesSize)
				{
					Normals[FB - FilteredVertexCount] = Normal;
				}
				if (C >= OldVerticesSize)
				{
					Normals[FC - FilteredVertexCount] = Normal;
				}
			}


			// Apply normals
			for (int32 i = 0; i < TransitionsVerticesSize; i++)
			{
				FVoxelProcMeshVertex& ProcMeshVertex = OutSection.ProcVertexBuffer[i + FilteredVertexCount];
				ProcMeshVertex.Normal = Normals[i].GetSafeNormal();
			}
		}
	}

	if (bEnableAmbientOcclusion)
	{
		SCOPE_CYCLE_COUNTER(STAT_AMBIENT_OCCLUSION);

		{
			Data->BeginGet();
			for (auto& Vertex : OutSection.ProcVertexBuffer)
			{
				int HitCount = 0;
				int TotalRays = 0;
				FRandomStream Stream(0 * (Vertex.Position.X * 29 + Vertex.Position.Y * 284736 + Vertex.Position.Z * 49994837 + ChunkPosition.X * 292 + ChunkPosition.Y * 2929 + ChunkPosition.Z * 29938 + Step() * 282));

				while (TotalRays < RayCount)
				{
					const float X = Stream.FRandRange(-1, 1);
					const float Y = Stream.FRandRange(-1, 1);
					const float Z = Stream.FRandRange(-1, 1);

					if (X * X + Y * Y + Z * Z > 1)
					{
						// Ignore ones outside unit
						continue;
					}

					if (FVector::DotProduct(FVector(X, Y, Z), Vertex.Normal) < 0)
					{
						// Ignore "down" directions
						continue;
					}

					const FVector Direction = FVector(X, Y, Z).GetSafeNormal();

					TotalRays++;
					for (int i = 1; i < RayMaxDistance; i++)
					{
						const FVector CurrentPosition = Vertex.Position + Direction * i * Step();
						float Value;
						FVoxelMaterial Dummy;
						GetValueAndMaterial(FMath::RoundToInt(CurrentPosition.X), FMath::RoundToInt(CurrentPosition.Y), FMath::RoundToInt(CurrentPosition.Z), Value, Dummy);

						if (Value <= 0)
						{
							HitCount++;
							break;
						}
					}
				}
				Vertex.Color.A = FMath::Clamp<int>(255.f * (1.f - HitCount / (float)TotalRays), 0, 255);
			}
			Data->EndGet();
		}
	}

	if (OutSection.ProcVertexBuffer.Num() < 3 || OutSection.ProcIndexBuffer.Num() == 0)
	{
		// Else physics thread crash
		OutSection.Reset();
	}
}

int FVoxelPolygonizer::Size()
{
	return 16 << Depth;
}

int FVoxelPolygonizer::Step()
{
	return 1 << Depth;
}


void FVoxelPolygonizer::GetValueAndMaterial(int X, int Y, int Z, float& OutValue, FVoxelMaterial& OutMaterial)
{
	//SCOPE_CYCLE_COUNTER(STAT_GETVALUEANDCOLOR);
	if ((X % Step() == 0) &&
		(Y % Step() == 0) &&
		(Z % Step() == 0) &&
		(0 <= X + 1 && X + 1 < (CHUNKSIZE + 3) * Step()) &&
		(0 <= Y + 1 && Y + 1 < (CHUNKSIZE + 3) * Step()) &&
		(0 <= Z + 1 && Z + 1 < (CHUNKSIZE + 3) * Step()))
	{
		GetValueAndMaterialFromCache(X / Step(), Y / Step(), Z / Step(), OutValue, OutMaterial);
	}
	else
	{
		GetValueAndMaterialNoCache(X, Y, Z, OutValue, OutMaterial);
	}
}

void FVoxelPolygonizer::GetValueAndMaterialNoCache(int X, int Y, int Z, float& OutValue, FVoxelMaterial& OutMaterial)
{
	Data->GetValueAndMaterial(X + ChunkPosition.X, Y + ChunkPosition.Y, Z + ChunkPosition.Z, OutValue, OutMaterial);
}

void FVoxelPolygonizer::GetValueAndMaterialFromCache(int X, int Y, int Z, float& OutValue, FVoxelMaterial& OutMaterial)
{
	const int I = X + 1;
	const int J = Y + 1;
	const int K = Z + 1;

	check(
		(0 <= I && I < CHUNKSIZE + 3) &&
		(0 <= J && J < CHUNKSIZE + 3) &&
		(0 <= K && K < CHUNKSIZE + 3));
	OutValue = CachedValues[I + (CHUNKSIZE + 3) * J + (CHUNKSIZE + 3) * (CHUNKSIZE + 3) * K];
	OutMaterial = CachedMaterials[I + (CHUNKSIZE + 3) * J + (CHUNKSIZE + 3) * (CHUNKSIZE + 3) * K];
}

void FVoxelPolygonizer::Get2DValueAndMaterial(EDirection Direction, int X, int Y, float& OutValue, FVoxelMaterial& OutMaterial)
{
	//SCOPE_CYCLE_COUNTER(STAT_GET2DVALUEANDCOLOR);
	int GX, GY, GZ;
	Local2DToGlobal(Size(), Direction, X, Y, 0, GX, GY, GZ);

	GetValueAndMaterial(GX, GY, GZ, OutValue, OutMaterial);
}


void FVoxelPolygonizer::SaveVertex(int X, int Y, int Z, short EdgeIndex, int Index)
{
	// +1: normals offset
	check(0 <= X + 1 && X + 1 < 18);
	check(0 <= Y + 1 && Y + 1 < 18);
	check(0 <= Z + 1 && Z + 1 < 18);
	check(0 <= EdgeIndex && EdgeIndex < 3);

	Cache[X + 1][Y + 1][Z + 1][EdgeIndex] = Index;
}

int FVoxelPolygonizer::LoadVertex(int X, int Y, int Z, short Direction, short EdgeIndex)
{
	bool XIsDifferent = static_cast<bool>((Direction & 0x01) != 0);
	bool YIsDifferent = static_cast<bool>((Direction & 0x02) != 0);
	bool ZIsDifferent = static_cast<bool>((Direction & 0x04) != 0);

	// +1: normals offset
	check(0 <= X - XIsDifferent + 1 && X - XIsDifferent + 1 < 18);
	check(0 <= Y - YIsDifferent + 1 && Y - YIsDifferent + 1 < 18);
	check(0 <= Z - ZIsDifferent + 1 && Z - ZIsDifferent + 1 < 18);
	check(0 <= EdgeIndex && EdgeIndex < 3);


	check(Cache[X - XIsDifferent + 1][Y - YIsDifferent + 1][Z - ZIsDifferent + 1][EdgeIndex] >= 0);
	return Cache[X - XIsDifferent + 1][Y - YIsDifferent + 1][Z - ZIsDifferent + 1][EdgeIndex];
}


void FVoxelPolygonizer::SaveVertex2D(EDirection Direction, int X, int Y, short EdgeIndex, int Index)
{
	if (EdgeIndex == 8 || EdgeIndex == 9)
	{
		// Never save those
		check(false);
	}

	check(0 <= X && X < 17);
	check(0 <= Y && Y < 17);
	check(0 <= EdgeIndex && EdgeIndex < 7);

	Cache2D[Direction][X][Y][EdgeIndex] = Index;
}

int FVoxelPolygonizer::LoadVertex2D(EDirection Direction, int X, int Y, short CacheDirection, short EdgeIndex)
{
	bool XIsDifferent = static_cast<bool>((CacheDirection & 0x01) != 0);
	bool YIsDifferent = static_cast<bool>((CacheDirection & 0x02) != 0);

	if (EdgeIndex == 8 || EdgeIndex == 9)
	{
		int Index;
		switch (Direction)
		{
		case XMin:
			Index = EdgeIndex == 8 ? 0 : 2;
			break;
		case XMax:
			Index = EdgeIndex == 8 ? 2 : 0;
			break;
		case YMin:
			Index = EdgeIndex == 8 ? 2 : 1;
			break;
		case YMax:
			Index = EdgeIndex == 8 ? 1 : 2;
			break;
		case ZMin:
			Index = EdgeIndex == 8 ? 1 : 0;
			break;
		case ZMax:
			Index = EdgeIndex == 8 ? 0 : 1;
			break;
		default:
			Index = 0;
			check(false);
			break;
		}
		int GX, GY, GZ;

		Local2DToGlobal(14, Direction, X - XIsDifferent, Y - YIsDifferent, -1, GX, GY, GZ);

		return LoadVertex(GX, GY, GZ, 0, Index);
	}

	check(0 <= X - XIsDifferent && X - XIsDifferent < 17);
	check(0 <= Y - YIsDifferent && Y - YIsDifferent < 17);
	check(0 <= EdgeIndex && EdgeIndex < 7);

	check(Cache2D[Direction][X - XIsDifferent][Y - YIsDifferent][EdgeIndex] >= 0);
	return Cache2D[Direction][X - XIsDifferent][Y - YIsDifferent][EdgeIndex];
}

void FVoxelPolygonizer::InterpolateX(int MinX, int MaxX, const int Y, const int Z, FVector& OutVector, uint8& OutAlpha)
{
	while (MaxX - MinX != 1)
	{
		check((MaxX + MinX) % 2 == 0);
		const int MiddleX = (MaxX + MinX) / 2;

		float ValueAtA;
		FVoxelMaterial MaterialAtA;
		GetValueAndMaterialNoCache(MinX, Y, Z, ValueAtA, MaterialAtA);

		float ValueAtMiddle;
		FVoxelMaterial MaterialAtMiddle;
		GetValueAndMaterialNoCache(MiddleX, Y, Z, ValueAtMiddle, MaterialAtMiddle);

		if ((ValueAtA > 0) == (ValueAtMiddle > 0))
		{
			// If min and middle have same sign
			MinX = MiddleX;
		}
		else
		{
			// If max and middle have same sign
			MaxX = MiddleX;
		}
	}

	// A: Min / B: Max
	float ValueAtA, ValueAtB;
	FVoxelMaterial MaterialAtA, MaterialAtB;
	GetValueAndMaterialNoCache(MinX, Y, Z, ValueAtA, MaterialAtA);
	GetValueAndMaterialNoCache(MaxX, Y, Z, ValueAtB, MaterialAtB);

	check(ValueAtA - ValueAtB != 0);
	const float t = ValueAtB / (ValueAtB - ValueAtA);

	OutVector = t * FVector(MinX, Y, Z) + (1 - t) *  FVector(MaxX, Y, Z);
	OutAlpha = t * MaterialAtA.Alpha + (1 - t) * MaterialAtB.Alpha;
}

void FVoxelPolygonizer::InterpolateY(const int X, int MinY, int MaxY, const int Z, FVector& OutVector, uint8& OutAlpha)
{
	while (MaxY - MinY != 1)
	{
		check((MaxY + MinY) % 2 == 0);
		const int MiddleY = (MaxY + MinY) / 2;

		float ValueAtA;
		FVoxelMaterial MaterialAtA;
		GetValueAndMaterialNoCache(X, MinY, Z, ValueAtA, MaterialAtA);

		float ValueAtMiddle;
		FVoxelMaterial MaterialAtMiddle;
		GetValueAndMaterialNoCache(X, MiddleY, Z, ValueAtMiddle, MaterialAtMiddle);

		if ((ValueAtA > 0) == (ValueAtMiddle > 0))
		{
			// If min and middle have same sign
			MinY = MiddleY;
		}
		else
		{
			// If max and middle have same sign
			MaxY = MiddleY;
		}
	}

	// A: Min / B: Max
	float ValueAtA, ValueAtB;
	FVoxelMaterial MaterialAtA, MaterialAtB;
	GetValueAndMaterialNoCache(X, MinY, Z, ValueAtA, MaterialAtA);
	GetValueAndMaterialNoCache(X, MaxY, Z, ValueAtB, MaterialAtB);

	check(ValueAtA - ValueAtB != 0);
	const float t = ValueAtB / (ValueAtB - ValueAtA);

	OutVector = t * FVector(X, MinY, Z) + (1 - t) *  FVector(X, MaxY, Z);
	OutAlpha = t * MaterialAtA.Alpha + (1 - t) * MaterialAtB.Alpha;
}

void FVoxelPolygonizer::InterpolateZ(const int X, const int Y, int MinZ, int MaxZ, FVector& OutVector, uint8& OutAlpha)
{
	while (MaxZ - MinZ != 1)
	{
		check((MaxZ + MinZ) % 2 == 0);
		const int MiddleZ = (MaxZ + MinZ) / 2;

		float ValueAtA;
		FVoxelMaterial MaterialAtA;
		GetValueAndMaterialNoCache(X, Y, MinZ, ValueAtA, MaterialAtA);

		float ValueAtMiddle;
		FVoxelMaterial MaterialAtMiddle;
		GetValueAndMaterialNoCache(X, Y, MiddleZ, ValueAtMiddle, MaterialAtMiddle);

		if ((ValueAtA > 0) == (ValueAtMiddle > 0))
		{
			// If min and middle have same sign
			MinZ = MiddleZ;
		}
		else
		{
			// If max and middle have same sign
			MaxZ = MiddleZ;
		}
	}

	// A: Min / B: Max
	float ValueAtA, ValueAtB;
	FVoxelMaterial MaterialAtA, MaterialAtB;
	GetValueAndMaterialNoCache(X, Y, MinZ, ValueAtA, MaterialAtA);
	GetValueAndMaterialNoCache(X, Y, MaxZ, ValueAtB, MaterialAtB);

	check(ValueAtA - ValueAtB != 0);
	const float t = ValueAtB / (ValueAtB - ValueAtA);

	OutVector = t * FVector(X, Y, MinZ) + (1 - t) *  FVector(X, Y, MaxZ);
	OutAlpha = t * MaterialAtA.Alpha + (1 - t) * MaterialAtB.Alpha;
}



void FVoxelPolygonizer::InterpolateX2D(EDirection Direction, int MinX, int MaxX, const int Y, FVector& OutVector, uint8& OutAlpha)
{
	while (MaxX - MinX != 1)
	{
		check((MaxX + MinX) % 2 == 0);
		int MiddleX = (MaxX + MinX) / 2;

		float ValueAtA;
		FVoxelMaterial MaterialAtA;
		Get2DValueAndMaterial(Direction, MinX, Y, ValueAtA, MaterialAtA);

		float ValueAtMiddle;
		FVoxelMaterial MaterialAtMiddle;
		Get2DValueAndMaterial(Direction, MiddleX, Y, ValueAtMiddle, MaterialAtMiddle);

		if ((ValueAtA > 0) == (ValueAtMiddle > 0))
		{
			// If min and middle have same sign
			MinX = MiddleX;
		}
		else
		{
			// If max and middle have same sign
			MaxX = MiddleX;
		}
	}

	// A: Min / B: Max
	float ValueAtA, ValueAtB;
	FVoxelMaterial MaterialAtA, MaterialAtB;
	Get2DValueAndMaterial(Direction, MinX, Y, ValueAtA, MaterialAtA);
	Get2DValueAndMaterial(Direction, MaxX, Y, ValueAtB, MaterialAtB);

	check(ValueAtA - ValueAtB != 0);
	const float t = ValueAtB / (ValueAtB - ValueAtA);

	int GMinX, GMaxX, GMinY, GMaxY, GMinZ, GMaxZ;
	Local2DToGlobal(Size(), Direction, MinX, Y, 0, GMinX, GMinY, GMinZ);
	Local2DToGlobal(Size(), Direction, MaxX, Y, 0, GMaxX, GMaxY, GMaxZ);

	OutVector = t * FVector(GMinX, GMinY, GMinZ) + (1 - t) *  FVector(GMaxX, GMaxY, GMaxZ);
	OutAlpha = t * MaterialAtA.Alpha + (1 - t) * MaterialAtB.Alpha;
}

void FVoxelPolygonizer::InterpolateY2D(EDirection Direction, int X, int MinY, int MaxY, FVector& OutVector, uint8& OutAlpha)
{
	while (MaxY - MinY != 1)
	{
		check((MaxY + MinY) % 2 == 0);
		const int MiddleY = (MaxY + MinY) / 2;

		float ValueAtA;
		FVoxelMaterial MaterialAtA;
		Get2DValueAndMaterial(Direction, X, MinY, ValueAtA, MaterialAtA);

		float ValueAtMiddle;
		FVoxelMaterial MaterialAtMiddle;
		Get2DValueAndMaterial(Direction, X, MiddleY, ValueAtMiddle, MaterialAtMiddle);

		if ((ValueAtA > 0) == (ValueAtMiddle > 0))
		{
			// If min and middle have same sign
			MinY = MiddleY;
		}
		else
		{
			// If max and middle have same sign
			MaxY = MiddleY;
		}
	}

	// A: Min / B: Max
	float ValueAtA, ValueAtB;
	FVoxelMaterial MaterialAtA, MaterialAtB;
	Get2DValueAndMaterial(Direction, X, MinY, ValueAtA, MaterialAtA);
	Get2DValueAndMaterial(Direction, X, MaxY, ValueAtB, MaterialAtB);

	check(ValueAtA - ValueAtB != 0);
	const float t = ValueAtB / (ValueAtB - ValueAtA);

	int GMinX, GMaxX, GMinY, GMaxY, GMinZ, GMaxZ;
	Local2DToGlobal(Size(), Direction, X, MinY, 0, GMinX, GMinY, GMinZ);
	Local2DToGlobal(Size(), Direction, X, MaxY, 0, GMaxX, GMaxY, GMaxZ);

	OutVector = t * FVector(GMinX, GMinY, GMinZ) + (1 - t) *  FVector(GMaxX, GMaxY, GMaxZ);
	OutAlpha = t * MaterialAtA.Alpha + (1 - t) * MaterialAtB.Alpha;
}

void FVoxelPolygonizer::GlobalToLocal2D(int Size, EDirection Direction, int GX, int GY, int GZ, int& OutLX, int& OutLY, int& OutLZ)
{
	const int S = Size;
	switch (Direction)
	{
	case XMin:
		OutLX = GY;
		OutLY = GZ;
		OutLZ = GX;
		break;
	case XMax:
		OutLX = GZ;
		OutLY = GY;
		OutLZ = S - GX;
		break;
	case YMin:
		OutLX = GZ;
		OutLY = GX;
		OutLZ = GY;
		break;
	case YMax:
		OutLX = GX;
		OutLY = GZ;
		OutLZ = S - GY;
		break;
	case ZMin:
		OutLX = GX;
		OutLY = GY;
		OutLZ = GZ;
		break;
	case ZMax:
		OutLX = GY;
		OutLY = GX;
		OutLZ = S - GZ;
		break;
	default:
		check(false)
			break;
	}
}

void FVoxelPolygonizer::Local2DToGlobal(int Size, EDirection Direction, int LX, int LY, int LZ, int& OutGX, int& OutGY, int& OutGZ)
{
	const int S = Size;
	switch (Direction)
	{
	case XMin:
		OutGX = LZ;
		OutGY = LX;
		OutGZ = LY;
		break;
	case XMax:
		OutGX = S - LZ;
		OutGY = LY;
		OutGZ = LX;
		break;
	case YMin:
		OutGX = LY;
		OutGY = LZ;
		OutGZ = LX;
		break;
	case YMax:
		OutGX = LX;
		OutGY = S - LZ;
		OutGZ = LY;
		break;
	case ZMin:
		OutGX = LX;
		OutGY = LY;
		OutGZ = LZ;
		break;
	case ZMax:
		OutGX = LY;
		OutGY = LX;
		OutGZ = S - LZ;
		break;
	default:
		check(false)
			break;
	}
}


FVector FVoxelPolygonizer::GetTranslated(const FVector& Vertex, const FVector& Normal)
{
	double DeltaX = 0;
	double DeltaY = 0;
	double DeltaZ = 0;

	if ((Vertex.X < KINDA_SMALL_NUMBER && !ChunkHasHigherRes[XMin]) || (Vertex.X > Size() - KINDA_SMALL_NUMBER && !ChunkHasHigherRes[XMax]) ||
		(Vertex.Y < KINDA_SMALL_NUMBER && !ChunkHasHigherRes[YMin]) || (Vertex.Y > Size() - KINDA_SMALL_NUMBER && !ChunkHasHigherRes[YMax]) ||
		(Vertex.Z < KINDA_SMALL_NUMBER && !ChunkHasHigherRes[ZMin]) || (Vertex.Z > Size() - KINDA_SMALL_NUMBER && !ChunkHasHigherRes[ZMax]))
	{
		return Vertex;
	}

	double TwoPowerK = 1 << Depth;
	double w = TwoPowerK / 4;

	if (ChunkHasHigherRes[XMin] && Vertex.X < Step())
	{
		DeltaX = (1 - static_cast<double>(Vertex.X) / TwoPowerK) * w;
	}
	if (ChunkHasHigherRes[XMax] && Vertex.X > 15 * Step())
	{
		DeltaX = (16 - 1 - static_cast<double>(Vertex.X) / TwoPowerK) * w;
	}
	if (ChunkHasHigherRes[YMin] && Vertex.Y < Step())
	{
		DeltaY = (1 - static_cast<double>(Vertex.Y) / TwoPowerK) * w;
	}
	if (ChunkHasHigherRes[YMax] && Vertex.Y > 15 * Step())
	{
		DeltaY = (16 - 1 - static_cast<double>(Vertex.Y) / TwoPowerK) * w;
	}
	if (ChunkHasHigherRes[ZMin] && Vertex.Z < Step())
	{
		DeltaZ = (1 - static_cast<double>(Vertex.Z) / TwoPowerK) * w;
	}
	if (ChunkHasHigherRes[ZMax] && Vertex.Z > 15 * Step())
	{
		DeltaZ = (16 - 1 - static_cast<double>(Vertex.Z) / TwoPowerK) * w;
	}

	FVector Q = FVector(
		(1 - Normal.X * Normal.X) * DeltaX - Normal.X * Normal.Y * DeltaY - Normal.X * Normal.Z * DeltaZ,
		-Normal.X * Normal.Y * DeltaX + (1 - Normal.Y * Normal.Y) * DeltaY - Normal.Y * Normal.Z * DeltaZ,
		-Normal.X * Normal.Z * DeltaX - Normal.Y * Normal.Z * DeltaY + (1 - Normal.Z * Normal.Z) * DeltaZ);

	return Vertex + Q;
}
