#pragma once
#include "DX3D11.h"
#include <cglm.h>

#define CHUNK_SIZE  16
#define CHUNK_SIZEV 32

typedef struct {
	uint16_t blockID;
	uint16_t blockstate;
}Block;

typedef struct {
	vertex* vertexList;
	int* indexlist;
}chunkMesh;

typedef struct {
	Block blocksState[CHUNK_SIZE][CHUNK_SIZEV][CHUNK_SIZE];
	chunkMesh mesh;
}Chunk;

void generateChunkMesh(Chunk* chunk);
void destroyBlock(Chunk* chunk);