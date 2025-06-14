#pragma once
#include "DX3D11.h"
#include <cglm.h>

#define CHUNK_SIZE  16
#define CHUNK_SIZEV 128

#define ISBLOCKSOLID(blockstate) (blockstate & 1)
#define SetBLOCKSOLID(blockstate) ((blockstate & 1) | 1)
#define UnsetBLOCKSOLID(blockstate) ((blockstate & 1) & ~1)

typedef struct {
	ID3D11Buffer* vertexBuffer;
	ID3D11Buffer* indexBuffer;
	int IndexListSize;
}chunkMesh;

typedef struct {
	uint16_t blockID;
	uint16_t blockstate;
}Block;

typedef struct{
	uint64_t isSolid[(CHUNK_SIZE*CHUNK_SIZEV*CHUNK_SIZE)/64];
	uint16_t BlockState[CHUNK_SIZE * CHUNK_SIZEV * CHUNK_SIZE];//first 10 bits represent a block id the following 2 bits are rotation values then 4 allowing 16 custom states;
}ChunkState;

typedef struct {
	int x;
	int z;
}chunkPos;

typedef struct {
	Block blocksState[CHUNK_SIZE][CHUNK_SIZEV][CHUNK_SIZE];
	chunkMesh mesh;
	chunkPos pos;
}Chunk;

typedef struct {
	ChunkBuffers* chunkBuffers;
	CRITICAL_SECTION* criticalSection;
	int x;
	int z;
	int ActiveIndex;
}chunkGenData;

DWORD WINAPI generateChunkMesh(chunkGenData* chunkGen);