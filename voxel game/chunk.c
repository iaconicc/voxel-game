#include "chunk.h"
#include "DX3D11.h"
#include "Blocks.h"
#include "BlockTexture.h"
#include <cglm.h>

#define MODULE L"chunk"
#include "Logger.h"

typedef struct {
	int index[6];
}face;

const static vertex cubeVertexs[8] = {
	{-0.5f, -0.5f, -0.5f, 0.0f, 0.0f}, //0
	{0.5f, -0.5f, -0.5f, 0.0f, 0.0f}, //1
	{0.5f, 0.5f, -0.5f, 0.0f, 0.0f}, //2
	{-0.5f, 0.5f, -0.5f, 0.0f, 0.0f}, //3
	{-0.5f, -0.5f, 0.5f, 0.0f, 0.0f}, //4
	{0.5f, -0.5f, 0.5f, 0.0f, 0.0f}, //5
	{0.5f, 0.5f,  0.5f, 0.0f, 0.0f}, //6
	{-0.5f, 0.5f, 0.5f, 0.0f, 0.0f}, //7
};

const static face cubeFaces[6] = {
	//0, 1, 2, 2, 1, 3
	{2, 3, 1, 0}, //south face
	{7, 6, 4, 5}, //north face
	{6, 7, 2, 3}, //top face
	{4, 5, 0, 1}, //bottom face
	{6, 2, 5, 1}, //west face
	{3, 7, 0, 4}, //east face
};

const static vec3 faceChecks[6] = {
	{0.0f,0.0f,-1.0f},
	{0.0f,0.0f,1.0f},
	{0.0f,1.0f,0.0f},
	{0.0f,-1.0f,0.0f},
	{1.0f,0.0f,0.0f},
	{-1.0f,0.0f,0.0f},
};

const static vec2 uvs[6][4] = {
	{{0.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f}}, //south face
	{{1.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f}, { 0.0f, 1.0f}}, //north face
	{{0.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f}}, //top face
	{{1.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f}, { 0.0f, 1.0f }}, //bottom face
	{{0.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f}}, //west face
	{{1.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}}, //east face
};

vertex* vertexlist;
int* indexlist;

#define CHUNK_SIZE  16
#define CHUNK_SIZEV 32

#define ISBLOCKSOLID(blockstate) (blockstate & 1)
#define SetBLOCKSOLID(blockstate) ((blockstate & 1) | 1)
#define UnsetBLOCKSOLID(blockstate) ((blockstate & 1) & ~1)

typedef struct{
	uint16_t blockID;
	uint16_t blockstate;
}Block;

typedef struct{
	vec3* vertexList;
	int* indexlist;
}chunkMesh;

typedef struct {
	Block blocksState[CHUNK_SIZE][CHUNK_SIZEV][CHUNK_SIZE];
}Chunk;

Chunk chunk;

static void populateVoxelMap(){
	for (size_t x = 0; x < CHUNK_SIZE; x++)
	{
		for (size_t y = 0; y < CHUNK_SIZEV; y++)
		{
			for (size_t z = 0; z < CHUNK_SIZE; z++)
			{
				chunk.blocksState[x][y][z].blockstate = UnsetBLOCKSOLID(chunk.blocksState[x][y][z].blockstate);
			}
		}
	}
}

static bool checkVoxel(vec3 pos)
{
	int x = (int) floorf(pos[0]);
	int y = (int) floorf(pos[1]);
	int z = (int) floorf(pos[2]);

	if (x < 0 || x > CHUNK_SIZE - 1 || y < 0 || y > CHUNK_SIZEV - 1 || z < 0 || z > CHUNK_SIZE - 1)
	{
		return false;
	}

	return ISBLOCKSOLID(chunk.blocksState[x][y][z].blockstate);
}

int currentVertexindex = 0;
int currentIndexListindex = 0;

static void addVoxelDataToChunk(vec3 pos)
{
		for (size_t f = 0; f < 6; f++)
		{
			vec3 blockTocheck;
			glm_vec3_add(pos, faceChecks[f], blockTocheck);

			if (!checkVoxel(blockTocheck)) {
				
				int baseIndex = currentVertexindex;

				//loop through each vertex and add a uv and vertex
				for (size_t v = 0; v < 4; v++)
				{
					//add vertex
					int FaceIndex = cubeFaces[f].index[v];
					glm_vec3_add(cubeVertexs[FaceIndex].pos, pos, vertexlist[currentVertexindex].pos);

					//scale the uv to the texture of first block
					vec2 ScaledUV = { 0 };
					ScaledUV[0] = GetUvOfOneBlockX();
					ScaledUV[1] = GetUvOfOneBlockY();
					glm_vec2_mul(ScaledUV, uvs[f][v], ScaledUV);

					//get block type then texture id to get Uv offsets for a texture in the atlas
					BlockType block = GetBlockTypeByID(2);
					vec2 UvOffsets = {0};
					GetUvOffsetByTexId(block.faces[f].textureId, &UvOffsets[0], &UvOffsets[1]);

					//add the offset to scaled uv
					glm_vec2_add(ScaledUV, UvOffsets, ScaledUV);

					glm_vec2_copy(ScaledUV, vertexlist[currentVertexindex].texPos);
					currentVertexindex++;
				}

				//add indexes so that it connects the added vertexs in clockwise order
				indexlist[currentIndexListindex++] = baseIndex;
				indexlist[currentIndexListindex++] = baseIndex + 3;
				indexlist[currentIndexListindex++] = baseIndex + 2;
				indexlist[currentIndexListindex++] = baseIndex;
				indexlist[currentIndexListindex++] = baseIndex + 1;
				indexlist[currentIndexListindex++] = baseIndex + 3;
			}
		}
}

void createBlock()
{
	populateVoxelMap();
	int indexSize = 0;
	int vertexSize = 0;

	//check each face and calculate how large the vertex and index list should be
	for (size_t x = 0; x < CHUNK_SIZE; x++)
	{
		for (size_t y = 0; y < CHUNK_SIZEV; y++)
		{
			for (size_t z = 0; z < CHUNK_SIZE; z++)
			{
				vec3 pos = { x,y,z };
				if (checkVoxel(pos))
				{
					for (size_t f = 0; f < 6; f++)
					{	
						vec3 blockToCheck;
						glm_vec3_add(pos, faceChecks[f], blockToCheck);
						if (!checkVoxel(blockToCheck))
						{
							indexSize += 6;
							vertexSize += 4;
						}
					}
				}
			}
		}
	}
	
	//allocate memory enough for both indices and vertexes
	indexlist = calloc(indexSize,sizeof(int));
	vertexlist = calloc(vertexSize,sizeof(vertex));

	LogDebug(L"vertexs: %u Bytes: %u indexes: %u Bytes: %u", vertexSize, ((sizeof(vertex) * vertexSize)/1000), indexSize, ((sizeof(int) * indexSize)/1000));

	//add vertexs and indices
	for (size_t x = 0; x < CHUNK_SIZE; x++)
	{
		for (size_t y = 0; y < CHUNK_SIZEV; y++)
		{
			for (size_t z = 0; z < CHUNK_SIZE; z++)
			{
				vec3 pos = { x, y, z};
				if (checkVoxel(pos))
				{
					addVoxelDataToChunk(pos);
				}
			}
		}
	}

	createVertexBufferAndAppendToList(vertexlist, sizeof(vertex) * vertexSize);
	createIndexDataBuffer(indexlist, sizeof(int) * indexSize);
}

void destroyBlock()
{
	free(vertexlist);
	free(indexlist);
}