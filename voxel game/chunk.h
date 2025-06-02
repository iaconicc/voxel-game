#pragma once
#include "DX3D11.h"
#include <cglm.h>

#define CHUNK_SIZE  16
#define CHUNK_SIZEV 32

#define ISBLOCKSOLID(blockstate) (blockstate & 1)
#define SetBLOCKSOLID(blockstate) ((blockstate & 1) | 1)
#define UnsetBLOCKSOLID(blockstate) ((blockstate & 1) & ~1)

typedef struct {
	uint16_t blockID;
	uint16_t blockstate;
}Block;

typedef struct {
	ID3D11Buffer* vertexBuffer;
	ID3D11Buffer* indexBuffer;
	int IndexListSize;
}chunkMesh;

typedef struct {
	int x;
	int z;
}chunkPos;

typedef struct {
	Block blocksState[CHUNK_SIZE][CHUNK_SIZEV][CHUNK_SIZE];
	chunkMesh mesh;
	chunkPos pos;
	int activeID;
}Chunk;

void WINAPI generateChunkMesh(void* lparam);