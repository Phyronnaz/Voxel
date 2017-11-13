#include "VoxelPrivatePCH.h"
#include "VoxelPolygonizerForCollisions.h"
#include "Transvoxel.h"
#include "VoxelData.h"
#include "VoxelData/Private/ValueOctree.h"
#include "VoxelMaterial.h"

DECLARE_CYCLE_STAT(TEXT("VoxelPolygonizerForCollisions ~ Main Iter"), STAT_MAIN_ITER_FC, STATGROUP_Voxel);
DECLARE_CYCLE_STAT(TEXT("VoxelPolygonizerForCollisions ~ CreateSection"), STAT_CREATE_SECTION_FC, STATGROUP_Voxel);

FVoxelPolygonizerForCollisions::FVoxelPolygonizerForCollisions(FVoxelData* Data, FIntVector ChunkPosition, const int SizeX, const int SizeY, const int SizeZ)
	: Data(Data)
	, ChunkPosition(ChunkPosition)
	, SizeX(SizeX)
	, SizeY(SizeY)
	, SizeZ(SizeZ)
	, LastOctree(nullptr)
{
	Cache.SetNumUninitialized(SizeX * SizeY * SizeZ * 3);
	CachedValues.SetNumUninitialized((SizeX + 1) * (SizeY + 1)* (SizeZ + 1));
	for (int i = 0; i < CachedValues.Num(); i++)
	{
		CachedValues[i] = -1;
	}
}

void FVoxelPolygonizerForCollisions::CreateSection(FProcMeshSection& OutSection)
{
	// Create forward lists
	std::forward_list<FVector> Vertices;
	std::forward_list<int32> Triangles;
	int VerticesSize = 0;
	int TrianglesSize = 0;

	Data->BeginGet();
	{
		SCOPE_CYCLE_COUNTER(STAT_MAIN_ITER_FC);
		// Iterate over cubes
		for (int X = 0; X < SizeX; X++)
		{
			for (int Y = 0; Y < SizeY; Y++)
			{
				for (int Z = 0; Z < SizeZ; Z++)
				{
					float CornerValues[8];

					CornerValues[0] = GetValue(X + 0, Y + 0, Z + 0);
					CornerValues[1] = GetValue(X + 1, Y + 0, Z + 0);
					CornerValues[2] = GetValue(X + 0, Y + 1, Z + 0);
					CornerValues[3] = GetValue(X + 1, Y + 1, Z + 0);
					CornerValues[4] = GetValue(X + 0, Y + 0, Z + 1);
					CornerValues[5] = GetValue(X + 1, Y + 0, Z + 1);
					CornerValues[6] = GetValue(X + 0, Y + 1, Z + 1);
					CornerValues[7] = GetValue(X + 1, Y + 1, Z + 1);

					const unsigned long CaseCode =
						((CornerValues[0] > 0) << 0)
						| ((CornerValues[1] > 0) << 1)
						| ((CornerValues[2] > 0) << 2)
						| ((CornerValues[3] > 0) << 3)
						| ((CornerValues[4] > 0) << 4)
						| ((CornerValues[5] > 0) << 5)
						| ((CornerValues[6] > 0) << 6)
						| ((CornerValues[7] > 0) << 7);

					if (CaseCode != 0 && CaseCode != 511)
					{
						// Cell has a nontrivial triangulation

						const FIntVector CornerPositions[8] = {
							FIntVector(X + 0, Y + 0, Z + 0),
							FIntVector(X + 1, Y + 0, Z + 0),
							FIntVector(X + 0, Y + 1, Z + 0),
							FIntVector(X + 1, Y + 1, Z + 0),
							FIntVector(X + 0, Y + 0, Z + 1),
							FIntVector(X + 1, Y + 0, Z + 1),
							FIntVector(X + 0, Y + 1, Z + 1),
							FIntVector(X + 1, Y + 1, Z + 1)
						};

						short ValidityMask = (X != 0) + 2 * (Y != 0) + 4 * (Z != 0);

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
								const float ValueAtA = CornerValues[IndexVerticeA];
								const float ValueAtB = CornerValues[IndexVerticeB];

								check(ValueAtA - ValueAtB != 0);
								const float t = ValueAtB / (ValueAtB - ValueAtA);

								FVector Q = t * static_cast<FVector>(PositionA) + (1 - t) * static_cast<FVector>(PositionB);


								VertexIndex = VerticesSize;

								Vertices.push_front(Q);
								VerticesSize++;
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
	}
	Data->EndGet();


	if (VerticesSize < 3)
	{
		// Early exit
		OutSection.Reset();
		return;
	}

	{
		SCOPE_CYCLE_COUNTER(STAT_CREATE_SECTION_FC);
		// Create section
		OutSection.Reset();
		OutSection.bEnableCollision = true;
		OutSection.bSectionVisible = true;
		OutSection.SectionLocalBox.Min = -FVector::OneVector;
		OutSection.SectionLocalBox.Max = FVector(SizeX, SizeY, SizeZ);
		OutSection.SectionLocalBox.IsValid = true;

		OutSection.ProcVertexBuffer.SetNum(VerticesSize);
		OutSection.ProcIndexBuffer.SetNumUninitialized(TrianglesSize);

		for (int i = 0; i < VerticesSize; i++)
		{
			FVector Vertice = Vertices.front();
			Vertices.pop_front();
			OutSection.ProcVertexBuffer[VerticesSize - 1 - i].Position = Vertice;
		}

		int i = 0;
		while (!Triangles.empty())
		{
			const int A = Triangles.front();
			Triangles.pop_front();
			const int B = Triangles.front();
			Triangles.pop_front();
			const int C = Triangles.front();
			Triangles.pop_front();

			OutSection.ProcIndexBuffer[i + 2] = A;
			OutSection.ProcIndexBuffer[i + 1] = B;
			OutSection.ProcIndexBuffer[i] = C;

			i += 3;
		}

	}

	check(Vertices.empty());
	check(Triangles.empty());

	if (OutSection.ProcVertexBuffer.Num() < 3 || OutSection.ProcIndexBuffer.Num() == 0)
	{
		// Else physics thread crash
		OutSection.Reset();
	}
}

float FVoxelPolygonizerForCollisions::GetValue(int X, int Y, int Z)
{
	check(0 <= X && X < SizeX + 1);
	check(0 <= Y && Y < SizeY + 1);
	check(0 <= Z && Z < SizeZ + 1);

	float& Value = CachedValues[X + (SizeX + 1) * Y + (SizeX + 1) * (SizeY + 1) * Z];

	if (Value == -1)
	{
		FVoxelMaterial Dummy;
		Data->GetValueAndMaterial(ChunkPosition.X + X, ChunkPosition.Y + Y, ChunkPosition.Z + Z, Value, Dummy);
	}
	return Value;
}

void FVoxelPolygonizerForCollisions::SaveVertex(int X, int Y, int Z, short EdgeIndex, int Index)
{
	check(0 <= X && X < SizeX);
	check(0 <= Y && Y < SizeY);
	check(0 <= Z && Z < SizeZ);
	check(0 <= EdgeIndex && EdgeIndex < 3);

	Cache[X + SizeX * Y + SizeX * SizeY * Z + SizeX * SizeY * SizeZ * EdgeIndex] = Index;
}

int FVoxelPolygonizerForCollisions::LoadVertex(int X, int Y, int Z, short Direction, short EdgeIndex)
{
	bool XIsDifferent = static_cast<bool>((Direction & 0x01) != 0);
	bool YIsDifferent = static_cast<bool>((Direction & 0x02) != 0);
	bool ZIsDifferent = static_cast<bool>((Direction & 0x04) != 0);

	check(0 <= X - XIsDifferent && X - XIsDifferent < SizeX);
	check(0 <= Y - YIsDifferent && Y - YIsDifferent < SizeY);
	check(0 <= Z - ZIsDifferent && Z - ZIsDifferent < SizeZ);
	check(0 <= EdgeIndex && EdgeIndex < 3);


	check(Cache[(X - XIsDifferent) + SizeX * (Y - YIsDifferent) + SizeX * SizeY * (Z - ZIsDifferent) + SizeX * SizeY * SizeZ * EdgeIndex] >= 0);
	return Cache[(X - XIsDifferent) + SizeX * (Y - YIsDifferent) + SizeX * SizeY * (Z - ZIsDifferent) + SizeX * SizeY * SizeZ * EdgeIndex];
}
