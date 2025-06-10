#include "world.h"
#include "App.h"
#include "Camera.h"
#include <processthreadsapi.h>
#include "hashmap.h"
#include "DX3D11.h"

#define MODULE L"WORLD"
#include "Logger.h"

struct hashmap* chunkHashmap;
CRITICAL_SECTION chunkmapMutex;

typedef (*JobFunction)(void* data);

typedef struct {
	JobFunction func;
	void* data;
}ChunkJob;

#define MAXWORKERTHREADS 8
#define MAXJOBS 8096
HANDLE WorkerThreads[MAXWORKERTHREADS];
FIFO* JobQueue = NULL;

CONDITION_VARIABLE WorkerSleepCondition;
CRITICAL_SECTION JobQueueMutex;

bool ThreadPoolRunning = true;

#define ViewDistance 8
#define WORLDSIZEINCHUNKS 64
#define WORLDSIZEINBLOCKS WORLDSIZEINCHUNKS * CHUNK_SIZE

Chunk chunk;
vec3 lastPlayerPos;

inline static void GetChunkPosFromAbsolutePos(vec3 pos, int* x, int* z)
{
	*x =(int) floorf(pos[0]/ CHUNK_SIZE);
	*z =(int) floorf(pos[2] / CHUNK_SIZE);
}

static bool ChunkIsInWorld(int x, int z) {
	return x >= 0 && x < WORLDSIZEINCHUNKS && z >= 0 && z < WORLDSIZEINCHUNKS;
}

static void RunChunkGen(void* data){
	chunkGenData* ChunkGenData = (chunkGenData*) data;
	generateChunkMesh(ChunkGenData);//the data is freed on generateChunkMesh() please do not double free data
}

static void QueueChunkJob(JobFunction func, void* data){
	ChunkJob job = {func, data};

	EnterCriticalSection(&JobQueueMutex);
	PushElement(&JobQueue, &job);
	WakeConditionVariable(&WorkerSleepCondition);
	LeaveCriticalSection(&JobQueueMutex);
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
					genData->x = x;
					genData->z = z;
					QueueChunkJob(RunChunkGen, genData);
				}
			}
		}
	}
}

static DWORD WINAPI ChunkJobWorkerTheads(LPVOID param){
	while (ThreadPoolRunning) {
		EnterCriticalSection(&JobQueueMutex);
		while (isFIFOEmpty(&JobQueue)) {
			SleepConditionVariableCS(&WorkerSleepCondition, &JobQueueMutex, INFINITE);

			if (!ThreadPoolRunning) {
				LeaveCriticalSection(&JobQueueMutex);
				return 0;
			}
		}

		ChunkJob* job = PopElement(&JobQueue);
		LeaveCriticalSection(&JobQueueMutex);

		if (job->func &&job->data){
			job->func(job->data);
		}

	}
	return 0;
}

static DWORD WINAPI WorldThread() {
	
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

	EnterCriticalSection(&JobQueueMutex);
	ThreadPoolRunning = false;
	WakeAllConditionVariable(&WorkerSleepCondition);
	LeaveCriticalSection(&JobQueueMutex);

	//close all worker threads
	for (int i = 0; i < MAXWORKERTHREADS; i++){
		WaitForSingleObject(WorkerThreads[i], INFINITE);
		CloseHandle(WorkerThreads[i]);
	}

	DeleteCriticalSection(&JobQueueMutex);
	DeleteCriticalSection(&chunkmapMutex);
	DestroyFIFO(&JobQueue);
	hashmap_free(chunkHashmap);
	return 0;
}

static bool BlockIsInWorld(int x, int y, int z) {
	return x >= 0 && x < WORLDSIZEINBLOCKS &&
		y < CHUNK_SIZEV &&
		z >= 0 && z < WORLDSIZEINBLOCKS;
}

#define CLAMP(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

void GetBlock(Block* block,int x, int y, int z){
	if (!BlockIsInWorld(x, y, z)){
		block->blockstate = UnsetBLOCKSOLID(block->blockstate);
		return;
	}
	else {

		if(y == 0){
			block->blockstate = SetBLOCKSOLID(block->blockstate);
			block->blockID = 4;
			return;
		}

		float r = sqrtf(x * x + z * z);
		float val = sin(r * 0.1f) + sin((x + z) * 0.1f);
		int sinx = CLAMP((int)(CHUNK_SIZEV * 0.25f * (2 + val)), 64, 150);

		if (y <= sinx) {
			if (y == sinx){
				block->blockID = 2;
			}
			else if(y >= (sinx-64)){
				block->blockID = 1;
			}
			block->blockstate = SetBLOCKSOLID(block->blockstate);
		}
		else {
			block->blockstate = UnsetBLOCKSOLID(block->blockstate);
		}

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
	InitializeCriticalSection(&JobQueueMutex);
	InitializeConditionVariable(&WorkerSleepCondition);
	InitFIFO(&JobQueue, MAXJOBS, sizeof(ChunkJob));
	
	//Create worker threads
	for (int i = 0; i < MAXWORKERTHREADS; i++){
		WorkerThreads[i] = CreateThread(0, 0, ChunkJobWorkerTheads, NULL, 0, NULL);
	}

	chunkHashmap = hashmap_new(sizeof(Chunk), 1024, 0, 0, chunkHash, chunkCompare, chunkFree, NULL);

	SetCamPos((vec3){0.0f, 128, 0.0f});
	getCameraTargetAndPosition(&lastPlayerPos, NULL);
	CheckViewDistance(lastPlayerPos);

	//chunk generation thread
	HANDLE handle;
	handle = CreateThread(0,0, WorldThread, 0, 0, NULL);

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

