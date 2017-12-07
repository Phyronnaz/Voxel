// Copyright 2017 Phyronnaz

#include "VoxelSave.h"
#include "BufferArchive.h"
#include "ArchiveSaveCompressedProxy.h"
#include "ArchiveLoadCompressedProxy.h"
#include "MemoryReader.h"



FVoxelChunkSave::FVoxelChunkSave()
	: Id(-1)
{

}

FVoxelChunkSave::FVoxelChunkSave(uint64 Id, FIntVector Position, float InValues[16 * 16 * 16], FVoxelMaterial InMaterials[16 * 16 * 16])
	: Id(Id)
{
	Values.SetNumUninitialized(16 * 16 * 16);
	Materials.SetNumUninitialized(16 * 16 * 16);

	for (int X = 0; X < 16; X++)
	{
		for (int Y = 0; Y < 16; Y++)
		{
			for (int Z = 0; Z < 16; Z++)
			{
				const int Index = X + 16 * Y + 16 * 16 * Z;
				Values[Index] = InValues[Index];
				Materials[Index] = InMaterials[Index];
			}
		}
	}
}

FVoxelWorldSave::FVoxelWorldSave()
	: Depth(-1)
{

}

void FVoxelWorldSave::Init(int NewDepth, const std::deque<TSharedRef<FVoxelChunkSave>>& ChunksList)
{
	Depth = NewDepth;

	FBufferArchive ToBinary;

	// Order matters
	for (auto Chunk : ChunksList)
	{
		ToBinary << *Chunk;
	}

	Data.Empty();
	FArchiveSaveCompressedProxy Compressor = FArchiveSaveCompressedProxy(Data, ECompressionFlags::COMPRESS_ZLIB);

	// Send entire binary array/archive to compressor
	Compressor << ToBinary;

	// Send archive serialized data to binary array
	Compressor.Flush();
}

std::deque<FVoxelChunkSave> FVoxelWorldSave::GetChunksList() const
{
	std::deque<FVoxelChunkSave> ChunksList;

	FArchiveLoadCompressedProxy Decompressor = FArchiveLoadCompressedProxy(Data, ECompressionFlags::COMPRESS_ZLIB);

	check(!Decompressor.GetError());

	//Decompress
	FBufferArchive DecompressedBinaryArray;
	Decompressor << DecompressedBinaryArray;

	FMemoryReader FromBinary = FMemoryReader(DecompressedBinaryArray);
	FromBinary.Seek(0);

	while (!FromBinary.AtEnd())
	{
		FVoxelChunkSave Chunk;
		FromBinary << Chunk;

		// Order matters
		ChunksList.push_back(Chunk);
	}

	return ChunksList;
}