#include "chunk.h"
#include "DX3D11.h"
#include <cglm.h>

typedef struct {
	unsigned int index[6];
}face;

const static vec3 cubeVertexs[8] = {
	{-0.5f, -0.5f, -0.5f},
	{0.5f, -0.5f, -0.5f},
	{0.5f, 0.5f, -0.5f},
	{-0.5f, 0.5f, -0.5f},
	{-0.5f, -0.5f, 0.5f},
	{0.5f, -0.5f, 0.5f},
	{0.5f, 0.5f,  0.5f},
	{-0.5f, 0.5f, 0.5f},
};

const static face cubeFaces[6] = {
	{0, 3, 1, 1, 3, 2}, //south face
	{5, 6, 4, 4, 6, 7}, //north face
	{3, 7, 2, 2, 7, 6}, //top face
	{1, 5, 0, 0, 5, 4}, //bottom face
	{4, 7, 0, 0, 7, 3}, //west face
	{1, 2, 5, 5, 2, 6}, //east face
};

vec3* vertexlist;
int* indexlist;

#define CHUNK_SIZE  16
#define CHUNK_SIZEV 32

typedef struct{
	bool isAir;
}Block;

typedef struct {
	Block blocks[CHUNK_SIZE][CHUNK_SIZEV][CHUNK_SIZE];
}Chunk;

int currentVertexindex = 0;

static void addVoxelDataToChunk(vec3 pos)
{
	for (size_t f = 0; f < 6; f++)
	{
		for (size_t v = 0; v < 6; v++)
		{
			int FaceIndex = cubeFaces[f].index[v];
			glm_vec3_add(cubeVertexs[FaceIndex], pos, vertexlist[currentVertexindex]);
			indexlist[currentVertexindex] = currentVertexindex;
			currentVertexindex++;
		}
	}
}

void createBlock()
{
	int indexSize = 0;
	int vertexSize = 0;
	//check each face and calculate how large the vertex and index list should be
	for (size_t x = 0; x < CHUNK_SIZE; x++)
	{
		for (size_t y = 0; y < CHUNK_SIZEV; y++)
		{
			for (size_t z = 0; z < CHUNK_SIZE; z++)
			{
				indexSize += 6;
				vertexSize += 8;
			}
		}
	}

	indexSize *= 6;
	vertexSize *= 8;

	//allocate memory enough for both indices and vertexes
	indexlist = malloc(sizeof(int) * indexSize);
	vertexlist = malloc(sizeof(vec3) * vertexSize);

	//add vertexs and indices

	for (size_t x = 0; x < CHUNK_SIZE; x++)
	{
		for (size_t y = 0; y < CHUNK_SIZEV; y++)
		{
			for (size_t z = 0; z < CHUNK_SIZE; z++)
			{
				vec3 pos = { x, y, z};
				addVoxelDataToChunk(pos);
			}
		}
	}


	createVertexBufferAndAppendToList(vertexlist, sizeof(vec3) * vertexSize);
	createIndexDataBuffer(indexlist, sizeof(int) * indexSize, indexSize);
}

void destroyBlock()
{
	free(vertexlist);
	free(indexlist);
}