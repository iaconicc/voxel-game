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
	{0, 0, 0, 1, 0, 0}, //0
	{1, 0, 0, 1, 0, 0}, //1
	{1, 1, 0, 1, 0, 0}, //2
	{0, 1, 0, 1, 0, 0}, //3
	{0, 0, 1, 1, 0, 0}, //4
	{1, 0, 1, 1, 0, 0}, //5
	{1, 1, 1, 1, 0, 0}, //6
	{0, 1, 1, 1, 0, 0}, //7
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

typedef struct {
	int8_t x;
	int8_t y;
	int8_t z;
}ChunkBlockOffset;

const static ChunkBlockOffset faceChecks[6] = {
	{0,0,-1},
	{0,0,1},
	{0,1,0},
	{0,-1,0},
	{1,0,0},
	{-1,0,0},
};

typedef struct {
	uint8_t u;
	uint8_t v;
}localUV;

const static localUV uvs[6][4] = {
	{{0, 0}, {1, 0}, {0, 1}, {1, 1}}, //south face
	{{1, 0}, {0, 0}, {1, 1}, {0, 1}}, //north face
	{{0, 0}, {1, 0}, {0, 1}, {1, 1}}, //top face
	{{1, 0}, {0, 0}, {1, 1}, {0, 1}}, //bottom face
	{{0, 0}, {1, 0}, {0, 1}, {1, 1}}, //west face
	{{1, 0}, {0, 0}, {1, 1}, {0, 1}}, //east face
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

inline static void populateVoxelMap(Chunk* chunk){
	for (int x = 0; x < CHUNK_SIZE; x++)
	{
		for (int y = 0; y < CHUNK_SIZE; y++)
		{
			for (int z = 0; z < CHUNK_SIZE; z++)
			{
				GetBlock(&chunk->blocksState[x][y][z], x + (chunk->pos.x * CHUNK_SIZE), y + (chunk->pos.y * CHUNK_SIZE), z + (chunk->pos.z * CHUNK_SIZE));
			}
		}
	}
}

inline static bool IsBlockInChunk(int x, int y, int z){
	if (x < 0 || x > CHUNK_SIZE - 1 || y < 0 || y > CHUNK_SIZE - 1 || z < 0 || z > CHUNK_SIZE - 1){
		return false;
	}
	else{
		return true;
	}
}

inline static bool checkVoxel(Chunk* chunk, ChunkBlockOffset pos)
{
	if (!IsBlockInChunk(pos.x, pos.y, pos.z)){
		Block block = {0};
		GetBlock(&block, pos.x + (chunk->pos.x * CHUNK_SIZE), pos.y + (chunk->pos.y * CHUNK_SIZE), pos.z + chunk->pos.z * CHUNK_SIZE);
		return ISBLOCKSOLID(block.blockstate);
	}

	return ISBLOCKSOLID(chunk->blocksState[pos.x][pos.y][pos.z].blockstate);
}

inline static void addVoxelDataToChunk(Chunk* chunk, ChunkBlockOffset pos, int* currentVertexindex, vertex* vertexList, int* indexList)
{	
		for (size_t f = 0; f < 6; f++)
		{
			ChunkBlockOffset blockToCheck = { pos.x + faceChecks[f].x, pos.y + faceChecks[f].y, pos.z + faceChecks[f].z};
			if (!checkVoxel(chunk, blockToCheck)) {
				uint16_t blockID = chunk->blocksState[pos.x][pos.y][pos.z].blockID;
				BlockType block = GetBlockTypeByID(blockID);
				int baseIndex = (*currentVertexindex);
				//loop through each vertex and add a uv and vertex
				for (size_t v = 0; v < 4; v++)
				{
					//add vertex
					int FaceIndex = cubeFaces[f].index[v];
					
					vertexList[(*currentVertexindex)].w = 1;
					vertexList[(*currentVertexindex)].x = cubeVertexs[FaceIndex].x + pos.x;
					vertexList[(*currentVertexindex)].y = cubeVertexs[FaceIndex].y + pos.y;
					vertexList[(*currentVertexindex)].z = cubeVertexs[FaceIndex].z + pos.z;

					//add local uv and textureID
					uint8_t uv = (uvs[f][v].u) | (uvs[f][v].v << 1);	
					vertexList[(*currentVertexindex)].texID = (uv << 14) | (block.directionalModels->faces[f].textureId);
#ifdef _DEBUG
					assert(uv < 4);
					assert(block.directionalModels->faces[f].textureId < 16384);
#endif // _DEBUG
					(*currentVertexindex)++;
				}
			}
		}
}

DWORD WINAPI generateChunkMesh(chunkGenData* chunkGen)
{
	ChunkBuffers* ActiveChunks= chunkGen->chunkBuffers;
	GPUBuffer* buffer = &ActiveChunks->BufferList[chunkGen->ActiveIndex];
	CRITICAL_SECTION* mutex = chunkGen->criticalSection;
	
	Chunk* chunk = calloc(1, sizeof(Chunk));
	if (!chunk){
		return -1;
	}

	chunk->mesh.IndexListSize = 0;
	int vertexListSize = 0;

	chunk->pos.x = chunkGen->x;
	chunk->pos.y = chunkGen->y;
	chunk->pos.z = chunkGen->z;

	populateVoxelMap(chunk);

	//check each face and calculate how large the vertex and index list should be
	for (size_t x = 0; x < CHUNK_SIZE; x++)
	{
		for (size_t y = 0; y < CHUNK_SIZE; y++)
		{
			for (size_t z = 0; z < CHUNK_SIZE; z++)
			{
				ChunkBlockOffset pos = { x,y,z };
				if (checkVoxel(chunk, pos))
				{
					for (size_t f = 0; f < 6; f++)
					{	
						ChunkBlockOffset blockToCheck = {x + faceChecks[f].x, y + faceChecks[f].y , z + faceChecks[f].z};
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
	
	if (vertexListSize == 0 || chunk->mesh.IndexListSize == 0){
		free(chunk);
		return -1;
	}

	//allocate memory enough for both indices and vertexes
	int* indexList = calloc(chunk->mesh.IndexListSize,sizeof(int));
	vertex* vertexList = calloc(vertexListSize,sizeof(vertex));
	
	int baseIndex = 0;
	int currentIndexListIndex = 0;
	for (int i = 0; i < vertexListSize / 4; i++) {
		int base = i * 4;
		indexList[i * 6 + 0] = base;
		indexList[i * 6 + 1] = base + 3;
		indexList[i * 6 + 2] = base + 2;
		indexList[i * 6 + 3] = base;
		indexList[i * 6 + 4] = base + 1;
		indexList[i * 6 + 5] = base + 3;
	}

	//LogDebug(L"vertexs: %u Bytes: %u indexes: %u Bytes: %u", vertexListSize, ((sizeof(vertex) * vertexListSize)/1000), chunk->mesh.IndexListSize, ((sizeof(int) * chunk->mesh.IndexListSize)/1000));

	int currentVertexindex = 0;
	int currentIndexListindex = 0;
	//add vertexs and indices
	for (size_t x = 0; x < CHUNK_SIZE; x++)
	{
		for (size_t y = 0; y < CHUNK_SIZE; y++)
		{
			for (size_t z = 0; z < CHUNK_SIZE; z++)
			{
				ChunkBlockOffset pos = { x, y, z};
				if (checkVoxel(chunk,pos))
				{
					addVoxelDataToChunk(chunk, pos, &currentVertexindex, vertexList, indexList);
				}
			}
		}
	}

	EnterCriticalSection(mutex);
	buffer->indexBufferElements = chunk->mesh.IndexListSize;
	buffer->vertexBufferInBytes = vertexListSize * sizeof(vertex);
	buffer->x = chunk->pos.x;
	buffer->y = chunk->pos.y;
	buffer->z = chunk->pos.z;
	updateBuffer(buffer, vertexList, indexList);
	buffer->inUse = true;
	LeaveCriticalSection(mutex);

	free(indexList);
	free(vertexList);
	free(chunk);
	free(chunkGen);
	return 0;
}