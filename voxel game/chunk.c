#include "chunk.h"

#include "Blocks.h"
#include "BlockTexture.h"
#include "App.h"
#include "hashmap.h"
#include <synchapi.h>
#include "FIFO.h"

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

const static vec2 uvs90[6][4] = {
	{{0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 1.0f}, {1.0f, 0.0f}}, // south face
	{{0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 1.0f}, {1.0f, 0.0f}}, // north face
	{{0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 1.0f}, {1.0f, 0.0f}}, // top face
	{{0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 1.0f}, {1.0f, 0.0f}}, // bottom face
	{{0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 1.0f}, {1.0f, 0.0f}}, // west face
	{{0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 1.0f}, {1.0f, 0.0f}}  // east face
};

const static vec2 uvs180[6][4] = {
	{{1.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f}}, // south face
	{{1.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f}}, // north face
	{{1.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f}}, // top face
	{{1.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f}}, // bottom face
	{{1.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f}}, // west face
	{{1.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f}}  // east face
};

const static vec2 uvs270[6][4] = {
	{{1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 1.0f}}, // south face
	{{1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 1.0f}}, // north face
	{{1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 1.0f}}, // top face
	{{1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 1.0f}}, // bottom face
	{{1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 1.0f}}, // west face
	{{1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 1.0f}}  // east face
};

static void populateVoxelMap(Chunk* chunk){
	for (size_t x = 0; x < CHUNK_SIZE; x++)
	{
		for (size_t y = 0; y < CHUNK_SIZEV; y++)
		{
			for (size_t z = 0; z < CHUNK_SIZE; z++)
			{
				chunk->blocksState[x][y][z].blockstate = SetBLOCKSOLID(chunk->blocksState[x][y][z].blockstate);
				GetBlock(&chunk->blocksState[x][y][z], x + (chunk->pos.x * CHUNK_SIZE), y, z + (chunk->pos.z * CHUNK_SIZE));
			}
		}
	}
}

static bool IsBlockInChunk(int x, int y, int z){
	if (x < 0 || x > CHUNK_SIZE - 1 || y < 0 || y > CHUNK_SIZEV - 1 || z < 0 || z > CHUNK_SIZE - 1){
		return false;
	}
	else{
		true;
	}
}

static bool checkVoxel(Chunk* chunk,vec3 pos)
{
	int x = (int) floorf(pos[0]);
	int y = (int) floorf(pos[1]);
	int z = (int) floorf(pos[2]);

	if (!IsBlockInChunk(x, y, z)){
		Block block;
		GetBlock(&block, x + (chunk->pos.x * CHUNK_SIZE), y, z + chunk->pos.z * CHUNK_SIZE);
		return ISBLOCKSOLID(block.blockstate);
	}

	return ISBLOCKSOLID(chunk->blocksState[x][y][z].blockstate);
}

static void addVoxelDataToChunk(Chunk* chunk, vec3 pos, int* currentVertexindex, int* currentIndexListIndex, vertex* vertexList, int* indexList)
{	
		for (size_t f = 0; f < 6; f++)
		{
			vec3 blockTocheck;
			glm_vec3_add(pos, faceChecks[f], blockTocheck);
			if (!checkVoxel(chunk, blockTocheck)) {
				uint16_t blockID = chunk->blocksState[(int)pos[0]][(int)pos[1]][(int)pos[2]].blockID;
				BlockType block = GetBlockTypeByID(blockID);
				int baseIndex = (*currentVertexindex);
				//loop through each vertex and add a uv and vertex
				for (size_t v = 0; v < 4; v++)
				{
					//add vertex
					int FaceIndex = cubeFaces[f].index[v];
					glm_vec3_add(cubeVertexs[FaceIndex].pos, pos, vertexList[(*currentVertexindex)].pos);

					//rotate the uv if needed
					vec2 rotatedUv;
					switch (block.directionalModels[0].faces[f].textureRotation)
					{
						case 90:
						{
							glm_vec2_copy(uvs90[f][v], rotatedUv);
							break;
						}
						case 180:
						{
							glm_vec2_copy(uvs180[f][v], rotatedUv);
							break;
						}
						case 270:
						{
							glm_vec2_copy(uvs270[f][v], rotatedUv);
							break;
						}
						default:
						{
							glm_vec2_copy(uvs[f][v], rotatedUv);
							break;
						}
					}

					//scale the uv to the texture of first block
					vec2 ScaledUV = { 0 };
					ScaledUV[0] = GetUvOfOneBlockX();
					ScaledUV[1] = GetUvOfOneBlockY();
					glm_vec2_mul(rotatedUv, ScaledUV, ScaledUV);

					//get Uv offsets for a texture in the atlas
					vec2 UvOffsets = {0};
					GetUvOffsetByTexId(block.directionalModels[0].faces[f].textureId, &UvOffsets[0], &UvOffsets[1]);

					//add the offset to scaled uv
					glm_vec2_add(ScaledUV, UvOffsets, ScaledUV);

					glm_vec2_copy(ScaledUV, vertexList[(*currentVertexindex)].texPos);
					(*currentVertexindex)++;
				}

				//add indexes so that it connects the added vertexs in clockwise order
				indexList[(*currentIndexListIndex)++] = baseIndex;
				indexList[(*currentIndexListIndex)++] = baseIndex + 3;
				indexList[(*currentIndexListIndex)++] = baseIndex + 2;
				indexList[(*currentIndexListIndex)++] = baseIndex;
				indexList[(*currentIndexListIndex)++] = baseIndex + 1;
				indexList[(*currentIndexListIndex)++] = baseIndex + 3;
			}
		}
}

DWORD WINAPI generateChunkMesh(chunkGenData* chunkGen)
{

	CRITICAL_SECTION* mutex = chunkGen->criticalSection;
	struct hashmap* chunkHashmap = chunkGen->hash;
	
	Chunk* chunk = calloc(1, sizeof(Chunk));
	if (!chunk){
		return -1;
	}

	chunk->mesh.IndexListSize = 0;
	int vertexListSize = 0;

	chunk->pos.x = chunkGen->x;
	chunk->pos.z = chunkGen->z;

	populateVoxelMap(chunk);

	//check each face and calculate how large the vertex and index list should be
	for (size_t x = 0; x < CHUNK_SIZE; x++)
	{
		for (size_t y = 0; y < CHUNK_SIZEV; y++)
		{
			for (size_t z = 0; z < CHUNK_SIZE; z++)
			{
				vec3 pos = { x,y,z };
				if (checkVoxel(chunk, pos))
				{
					for (size_t f = 0; f < 6; f++)
					{	
						vec3 blockToCheck;
						glm_vec3_add(pos, faceChecks[f], blockToCheck);
						if (!checkVoxel(chunk, blockToCheck))
						{
							chunk->mesh.IndexListSize += 6;
							vertexListSize += 4;
						}
					}
				}
			}
		}
	}
	
	//allocate memory enough for both indices and vertexes
	int* indexList = calloc(chunk->mesh.IndexListSize,sizeof(int));
	vertex* vertexList = calloc(vertexListSize,sizeof(vertex));

	LogDebug(L"vertexs: %u Bytes: %u indexes: %u Bytes: %u", vertexListSize, ((sizeof(vertex) * vertexListSize)/1000), chunk->mesh.IndexListSize, ((sizeof(int) * chunk->mesh.IndexListSize)/1000));

	int currentVertexindex = 0;
	int currentIndexListindex = 0;
	//add vertexs and indices
	for (size_t x = 0; x < CHUNK_SIZE; x++)
	{
		for (size_t y = 0; y < CHUNK_SIZEV; y++)
		{
			for (size_t z = 0; z < CHUNK_SIZE; z++)
			{
				vec3 pos = { x, y, z};
				if (checkVoxel(chunk,pos))
				{
					addVoxelDataToChunk(chunk, pos, &currentVertexindex, &currentIndexListindex, vertexList, indexList);
				}
			}
		}
	}

	chunk->mesh.indexBuffer = createIndexDataBuffer(indexList, (chunk->mesh.IndexListSize*sizeof(int)));
	chunk->mesh.vertexBuffer = createVertexBuffer(vertexList, (vertexListSize * sizeof(vertex)));

	EnterCriticalSection(mutex);
	hashmap_set(chunkHashmap, chunk);
	LeaveCriticalSection(mutex);

	free(indexList);
	free(vertexList);
	free(chunk);
	free(chunkGen);
	return 0;
}