#include "world.h"
#include "chunk.h"
#include "App.h"
#include "Camera.h"
#include <processthreadsapi.h>
#include "hashmap.h"
#include "DX3D11.h"

#define MODULE L"WORLD"
#include "Logger.h"

struct hashmap* chunkHashmap;

vec3 lastPlayerPos;

CRITICAL_SECTION chunkmapMutex;

Chunk chunk;
bool drawing = false;

static WINAPI WorldThread() {

	while (ProgramIsRunning())
	{
		if (!DxsettingUp())
		{
			vec3 currentPlayerPos;
			getCameraTargetAndPosition(&currentPlayerPos, NULL);
			if (currentPlayerPos[0] != lastPlayerPos[0] || currentPlayerPos[1] != lastPlayerPos[1] || currentPlayerPos[2] != lastPlayerPos[2]){
				for (int x = 0; x < 16; x++)
				{
					for (int z = 0; z < 16; z++)
					{
						chunk.pos.x = x;
						chunk.pos.z = z;
						Chunk* existingChunk = hashmap_get(chunkHashmap, &chunk);
						if (!existingChunk)
						{
							chunkGenData* genData =  malloc(sizeof(chunkGenData));
							genData->criticalSection = &chunkmapMutex;
							genData->hash = chunkHashmap;
							genData->x = x;
							genData->z = z;
							CreateThread(0, 0, generateChunkMesh, genData, 0, NULL);
						}
					}
				}
				glm_vec3_copy(currentPlayerPos, lastPlayerPos);
			}
		}
	}

	hashmap_free(chunkHashmap);
	return 0;
}

static uint64_t chunkHash(const void* item, uint64_t seed0, uint64_t seed1) {
    Chunk* chunk = (Chunk*)item;  
    return hashmap_sip((const void*)&chunk->pos, sizeof(chunkPos), seed0, seed1);  
}

static int chunkCompare(const void* a, const void* b, void* udata)
{
	const Chunk* chunkA = (Chunk*) a;
	const Chunk* chunkB = (Chunk*) b;
	return memcmp((const void*) &chunkA->pos, (const void*) &chunkB->pos, sizeof(chunkPos));
}

static void chunkFree(void* item)
{
	const Chunk* chunk = (Chunk*) item;
	if (chunk->mesh.indexlist){
		free(chunk->mesh.indexlist);
	}
	if (chunk->mesh.vertexList){
		free(chunk->mesh.vertexList);
	}
}

CRITICAL_SECTION* getChunkmapMutex()
{
	return &chunkmapMutex;
}

void StartWorld()
{
	InitializeCriticalSection(&chunkmapMutex);

	getCameraTargetAndPosition(&lastPlayerPos, NULL);
	//ensure that chunks are generated at least once
	lastPlayerPos[0] = lastPlayerPos[0]+1;

	chunkHashmap = hashmap_new(sizeof(Chunk), 1089, 0, 0, chunkHash, chunkCompare, chunkFree, NULL);

	DWORD id;
	HANDLE handle;
	//chunk generation thread
	handle = CreateThread(0,0, WorldThread, 0, 0, &id);
}

static bool sendChunkData(const void* item, void* udata){
	Chunk* chunk = (Chunk*) item;
	vec3 pos = { ((float)(chunk->pos.x)) * 16, 0.0f, ((float)(chunk->pos.z)) * 16 };
	DrawMesh(chunk->mesh.vertexList, chunk->mesh.indexlist, chunk->mesh.vertexListSize * sizeof(vertex), chunk->mesh.IndexListSize * sizeof(int), pos);
	return true;
}

void DrawChunks()
{
	if (chunkHashmap) {
		Chunk* chunk = NULL;
		size_t i = 0;

		EnterCriticalSection(&chunkmapMutex);

		while (hashmap_iter(chunkHashmap, &i, &chunk)) {
			if (chunk->chunkIsReady) {
				vec3 pos = { ((float)(chunk->pos.x)) * 16, 0.0f, ((float)(chunk->pos.z)) * 16 };
				DrawMesh(chunk->mesh.vertexList, chunk->mesh.indexlist, chunk->mesh.vertexListSize * sizeof(vertex), chunk->mesh.IndexListSize * sizeof(int), pos);
			}
		}

		//hashmap_scan(chunkHashmap, sendChunkData, NULL);
		LeaveCriticalSection(&chunkmapMutex);
	}
}

