#include "world.h"
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

#define MAXTHREADS 100
#define MAXJOBS 1024

FIFO* JobQueue = NULL;
FIFO* AvailableThread = NULL;

int ThreadsTaken = 0;

#define ViewDistance 32
#define WORLDSIZEINCHUNKS 128
#define WORLDSIZEINBLOCKS WORLDSIZEINCHUNKS * CHUNK_SIZE

typedef struct {
	chunkGenData* ChunkGenData;
	bool delete;
}ChunkJob;


inline static void GetChunkPosFromAbsolutePos(vec3 pos, int* x, int* z)
{
	*x =(int) floorf(pos[0]/ CHUNK_SIZE);
	*z =(int) floorf(pos[2] / CHUNK_SIZE);
}

static bool ChunkIsInWorld(int x, int z) {
	return x >= 0 && x < WORLDSIZEINCHUNKS && z >= 0 && z < WORLDSIZEINCHUNKS;
}

static void CheckViewDistance(vec3 pos){
	int Chunkx = 0, Chunkz = 0;
	GetChunkPosFromAbsolutePos(pos, &Chunkx, &Chunkz);

	for (int x = Chunkx - ViewDistance; x < Chunkx + ViewDistance; x++){
		for (int z = Chunkz - ViewDistance; z < Chunkz + ViewDistance; z++){
			if (ChunkIsInWorld(x, z)){
				chunk.pos.x = x;
				chunk.pos.z = z;
				Chunk* existingChunk;
				if (!(existingChunk = hashmap_get(chunkHashmap, &chunk))){
					chunkGenData* genData = calloc(1,sizeof(chunkGenData));
					genData->criticalSection = &chunkmapMutex;
					genData->hash = chunkHashmap;
					genData->ThreadQueue = AvailableThread;
					genData->x = x;
					genData->z = z;
					ChunkJob job = {genData};
					job.delete = false;
					PushElement(&JobQueue, &job);
				}
			}
		}
	}
}

static WINAPI ChunkJobThread(){
	while (ProgramIsRunning()){
		if (!isFIFOEmpty(&JobQueue)){
			if(!isFIFOEmpty(&AvailableThread)){
				int* Thread =(int*) PopElement(&AvailableThread);
				ChunkJob* chunkJob =(ChunkJob*) PopElement(&JobQueue);
				chunkJob->ChunkGenData->ThreadID = *Thread;
				CreateThread(0, 0, generateChunkMesh, chunkJob->ChunkGenData, 0, NULL);
			}
		}
	}
	return 0;
}

static WINAPI WorldThread() {

	CreateThread(0, 0, ChunkJobThread, NULL, 0, NULL);

	while (ProgramIsRunning())
	{
		if (!DxsettingUp())
		{
			vec3 currentPlayerPos;
			getCameraTargetAndPosition(&currentPlayerPos, NULL);
			
			int LastChunkx = 0;
			int LastChunkz = 0;

			int Chunkx = 0;
			int Chunkz = 0;

			GetChunkPosFromAbsolutePos(lastPlayerPos, &LastChunkx, &LastChunkz);
			GetChunkPosFromAbsolutePos(currentPlayerPos, &Chunkx, &Chunkz);
			
			if (!(Chunkx == LastChunkx && Chunkz == LastChunkz)){
				CheckViewDistance(currentPlayerPos);
			}

			glm_ivec3_copy(currentPlayerPos, lastPlayerPos);
		}
	}

	DestroyFIFO(&JobQueue);
	DestroyFIFO(&AvailableThread);
	hashmap_free(chunkHashmap);
	return 0;
}

static bool BlockIsInWorld(int x, int y, int z) {
	return x >= 0 && x < WORLDSIZEINBLOCKS &&
		y >= 0 && y < CHUNK_SIZEV &&
		z >= 0 && z < WORLDSIZEINBLOCKS;
}

void GetBlock(Block* block,int x, int y, int z){
	if (!BlockIsInWorld(x, y, z)){
		block->blockstate = UnsetBLOCKSOLID(block->blockstate);
		return;
	}else{
		block->blockstate = SetBLOCKSOLID(block->blockstate);
	}

	if (y == 0)
	{
		block->blockID = 4;
	}
	if (y > 0)
	{
		block->blockID = 0;
	}
	if (y > CHUNK_SIZEV - 5)
	{
		block->blockID = 1;
	}
	if (y == CHUNK_SIZEV - 1)
	{
		block->blockID = 2;
	}
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
	if (chunk->mesh.indexBuffer){
		chunk->mesh.indexBuffer->lpVtbl->Release(chunk->mesh.indexBuffer);
	}
	if (chunk->mesh.vertexBuffer){
		chunk->mesh.vertexBuffer->lpVtbl->Release(chunk->mesh.vertexBuffer);
	}
}

CRITICAL_SECTION* getChunkmapMutex()
{
	return &chunkmapMutex;
}

HANDLE StartWorld()
{
	InitializeCriticalSection(&chunkmapMutex);
	InitFIFO(&JobQueue, MAXJOBS, sizeof(ChunkJob));
	InitFIFO(&AvailableThread, MAXTHREADS, sizeof(int));
	
	for (int i = 0; i < MAXTHREADS; i++){
		PushElement(&AvailableThread, &i);
	}

	chunkHashmap = hashmap_new(sizeof(Chunk), 16384, 0, 0, chunkHash, chunkCompare, chunkFree, NULL);

	SetCamPos((vec3){(float)WORLDSIZEINBLOCKS /2, 33, (float)WORLDSIZEINBLOCKS / 2});
	getCameraTargetAndPosition(&lastPlayerPos, NULL);
	CheckViewDistance(lastPlayerPos);

	DWORD id;
	HANDLE handle;
	//chunk generation thread
	handle = CreateThread(0,0, WorldThread, 0, 0, &id);

	return handle;
}

void DrawChunks()
{
	if (chunkHashmap) {
		Chunk* chunk = NULL;
		size_t i = 0;
		
		EnterCriticalSection(&chunkmapMutex);
		while (hashmap_iter(chunkHashmap, &i, &chunk)) {
				vec3 pos = { ((float)(chunk->pos.x)) * (float) CHUNK_SIZE, 0.0f, ((float)(chunk->pos.z)) * (float) CHUNK_SIZE};
				DrawMesh(chunk->mesh.vertexBuffer, chunk->mesh.indexBuffer, chunk->mesh.IndexListSize, pos);
		};
		LeaveCriticalSection(&chunkmapMutex);
	}
}

