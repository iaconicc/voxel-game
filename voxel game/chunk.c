#include "chunk.h"

#include "Blocks.h"
#include "BlockTexture.h"

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

#define ISBLOCKSOLID(blockstate) (blockstate & 1)
#define SetBLOCKSOLID(blockstate) ((blockstate & 1) | 1)
#define UnsetBLOCKSOLID(blockstate) ((blockstate & 1) & ~1)

Chunk testChunk;

static void populateVoxelMap(Chunk* chunk){
	for (size_t x = 0; x < CHUNK_SIZE; x++)
	{
		for (size_t y = 0; y < CHUNK_SIZEV; y++)
		{
			for (size_t z = 0; z < CHUNK_SIZE; z++)
			{
				chunk->blocksState[x][y][z].blockstate = SetBLOCKSOLID(chunk->blocksState[x][y][z].blockstate);
			}
		}
	}
}

static bool checkVoxel(Chunk* chunk,vec3 pos)
{
	int x = (int) floorf(pos[0]);
	int y = (int) floorf(pos[1]);
	int z = (int) floorf(pos[2]);

	if (x < 0 || x > CHUNK_SIZE - 1 || y < 0 || y > CHUNK_SIZEV - 1 || z < 0 || z > CHUNK_SIZE - 1)
	{
		return false;
	}

	return ISBLOCKSOLID(chunk->blocksState[x][y][z].blockstate);
}

static void addVoxelDataToChunk(Chunk* chunk, vec3 pos, int* currentVertexindex, int* currentIndexListIndex)
{	
		for (size_t f = 0; f < 6; f++)
		{
			vec3 blockTocheck;
			glm_vec3_add(pos, faceChecks[f], blockTocheck);
			if (!checkVoxel(&testChunk, blockTocheck)) {
				BlockType block = GetBlockTypeByID(3);
				int baseIndex = (*currentVertexindex);
				//loop through each vertex and add a uv and vertex
				for (size_t v = 0; v < 4; v++)
				{
					//add vertex
					int FaceIndex = cubeFaces[f].index[v];
					glm_vec3_add(cubeVertexs[FaceIndex].pos, pos, chunk->mesh.vertexList[(*currentVertexindex)].pos);

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

					glm_vec2_copy(ScaledUV, chunk->mesh.vertexList[(*currentVertexindex)].texPos);
					(*currentVertexindex)++;
				}

				//add indexes so that it connects the added vertexs in clockwise order
				chunk->mesh.indexlist[(*currentIndexListIndex)++] = baseIndex;
				chunk->mesh.indexlist[(*currentIndexListIndex)++] = baseIndex + 3;
				chunk->mesh.indexlist[(*currentIndexListIndex)++] = baseIndex + 2;
				chunk->mesh.indexlist[(*currentIndexListIndex)++] = baseIndex;
				chunk->mesh.indexlist[(*currentIndexListIndex)++] = baseIndex + 1;
				chunk->mesh.indexlist[(*currentIndexListIndex)++] = baseIndex + 3;
			}
		}
}

void generateChunkMesh(Chunk* chunk)
{
	int indexSize = 0;
	int vertexSize = 0;

	populateVoxelMap(&testChunk);

	//check each face and calculate how large the vertex and index list should be
	for (size_t x = 0; x < CHUNK_SIZE; x++)
	{
		for (size_t y = 0; y < CHUNK_SIZEV; y++)
		{
			for (size_t z = 0; z < CHUNK_SIZE; z++)
			{
				vec3 pos = { x,y,z };
				if (checkVoxel(&testChunk, pos))
				{
					for (size_t f = 0; f < 6; f++)
					{	
						vec3 blockToCheck;
						glm_vec3_add(pos, faceChecks[f], blockToCheck);
						if (!checkVoxel(&testChunk, blockToCheck))
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
	testChunk.mesh.indexlist = calloc(indexSize,sizeof(int));
	testChunk.mesh.vertexList = calloc(vertexSize,sizeof(vertex));

	LogDebug(L"vertexs: %u Bytes: %u indexes: %u Bytes: %u", vertexSize, ((sizeof(vertex) * vertexSize)/1000), indexSize, ((sizeof(int) * indexSize)/1000));

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
				if (checkVoxel(&testChunk,pos))
				{
					addVoxelDataToChunk(&testChunk, pos, &currentVertexindex, &currentIndexListindex);
				}
			}
		}
	}

	createVertexBufferAndAppendToList(testChunk.mesh.vertexList, sizeof(vertex) * vertexSize);
	createIndexDataBuffer(testChunk.mesh.indexlist, sizeof(int) * indexSize);
}

void destroyBlock(Chunk* chunk)
{
	free(testChunk.mesh.indexlist);
	free(testChunk.mesh.vertexList);
}