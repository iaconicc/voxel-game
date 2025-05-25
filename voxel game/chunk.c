#include "chunk.h"
#include "DX3D11.h"
#include <cglm.h>

#define MODULE L"chunk"
#include "Logger.h"

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
	//0, 1, 2, 2, 1, 3
	{0, 3, 1, 2}, //south face
	{5, 6, 4, 7}, //north face
	{3, 7, 2, 6}, //top face
	{1, 5, 0, 4}, //bottom face
	{4, 7, 0, 3}, //west face
	{1, 2, 5, 6}, //east face
};

const static vec3 faceChecks[6] = {
	{0.0f,0.0f,-1.0f},
	{0.0f,0.0f,1.0f},
	{0.0f,1.0f,0.0f},
	{0.0f,-1.0f,0.0f},
	{-1.0f,0.0f,0.0f},
	{1.0f,0.0f,0.0f},
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

Chunk chunk;

static void populateVoxelMap(){
	for (size_t x = 0; x < CHUNK_SIZE; x++)
	{
		for (size_t y = 0; y < CHUNK_SIZEV; y++)
		{
			for (size_t z = 0; z < CHUNK_SIZE; z++)
			{
				chunk.blocks[x][y][z].isAir = false;
			}
		}
	}
}

static bool checkVoxel(vec3 pos)
{
	int x =(int) floorf(pos[0]);
	int y = (int)floorf(pos[1]);
	int z = (int)floorf(pos[2]);

	if (x < 0 || x > CHUNK_SIZE - 1 || y < 0 || y > CHUNK_SIZEV - 1 || z < 0 || z > CHUNK_SIZE - 1)
	{
		return true;
	}

	return chunk.blocks[x][y][z].isAir;
}

int currentVertexindex = 0;
int currentIndexListindex = 0;

static void addVoxelDataToChunk(vec3 pos)
{
	
		for (size_t f = 0; f < 6; f++)
		{
			vec3 blockTocheck;
			glm_vec3_add(pos, faceChecks[f], blockTocheck);

			if (checkVoxel(blockTocheck)) {
				
				int baseIndex = currentVertexindex;
				indexlist[currentIndexListindex++] = baseIndex;
				indexlist[currentIndexListindex++] = baseIndex + 1;
				indexlist[currentIndexListindex++] = baseIndex + 2;
				indexlist[currentIndexListindex++] = baseIndex + 2;
				indexlist[currentIndexListindex++] = baseIndex + 1;
				indexlist[currentIndexListindex++] = baseIndex + 3;

				for (size_t v = 0; v < 4; v++)
				{
					int FaceIndex = cubeFaces[f].index[v];
					glm_vec3_add(cubeVertexs[FaceIndex], pos, vertexlist[currentVertexindex]);
					currentVertexindex++;
				}

			}
		}
	
}

void createBlock()
{
	int indexSize = 0;
	int vertexSize = 0;

	//vertexSize = 36 * CHUNK_SIZE * CHUNK_SIZEV * CHUNK_SIZE; // 6 faces, 6 vertices per face
	//indexSize = 36 * CHUNK_SIZE * CHUNK_SIZEV * CHUNK_SIZE;

	//check each face and calculate how large the vertex and index list should be
	for (size_t x = 0; x < CHUNK_SIZE; x++)
	{
		for (size_t y = 0; y < CHUNK_SIZEV; y++)
		{
			for (size_t z = 0; z < CHUNK_SIZE; z++)
			{
				for (size_t f = 0; f < 6; f++)
				{
					vec3 pos = {x,y,z};
					vec3 blockToCheck;
					glm_vec3_add(pos, faceChecks[f], blockToCheck);
					if (checkVoxel(blockToCheck))
					{
						indexSize += 6;
						vertexSize += 4;
					}
				}
			}
		}
	}

	
	//allocate memory enough for both indices and vertexes
	indexlist = malloc(sizeof(int) * indexSize);
	vertexlist = malloc(sizeof(vec3) * vertexSize);

	LogDebug(L"vertexs: %u Bytes: %u indexes: %u Bytes: %u", vertexSize, ((sizeof(vec3) * vertexSize)/1000), indexSize, ((sizeof(int) * indexSize)/1000));

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
	createIndexDataBuffer(indexlist, sizeof(int) * indexSize);
}

void destroyBlock()
{
	free(vertexlist);
	free(indexlist);
}