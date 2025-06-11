#include "world.h"
#include "App.h"
#include "Camera.h"
#include <processthreadsapi.h>
#include "hashmap.h"
#include "DX3D11.h"

#define MODULE L"WORLD"
#include "Logger.h"

CRITICAL_SECTION activeListMutex;
ChunkBuffers* activeList;

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
#define ACTIVE_GRID_SIZE (2 * ViewDistance)
#define BUFFER_INDEX(x, z) ((ACTIVE_GRID_SIZE * (x)) + (z))

#define WORLDSIZEINCHUNKS 128
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
				int localX = x - (Chunkx - ViewDistance);
				int localZ = z - (Chunkz - ViewDistance);
				int index = BUFFER_INDEX(localX, localZ);
				if (activeList->BufferList[index].x != x || activeList->BufferList[index].z != z) {
					activeList->BufferList[index].inUse = false;
				}

				if (!activeList->BufferList[index].inUse) {
					chunkGenData* genData = calloc(1, sizeof(chunkGenData));
					genData->criticalSection = &activeListMutex;
					genData->chunkBuffers = activeList;
					genData->x = x;
					genData->z = z;
					genData->ActiveX = localX;
					genData->ActiveZ = localZ;
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
	DeleteCriticalSection(&activeListMutex);
	DestroyFIFO(&JobQueue);
	ReleaseChunkBuffers(activeList);
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
		int sinx = CLAMP((int)(CHUNK_SIZEV * 0.25f * (2 + val)), 32, 126);

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

HANDLE StartWorld()
{
	InitializeCriticalSection(&activeListMutex);
	InitializeCriticalSection(&JobQueueMutex);
	InitializeConditionVariable(&WorkerSleepCondition);
	InitFIFO(&JobQueue, MAXJOBS, sizeof(ChunkJob));
	
	//Create worker threads
	for (int i = 0; i < MAXWORKERTHREADS; i++){
		WorkerThreads[i] = CreateThread(0, 0, ChunkJobWorkerTheads, NULL, 0, NULL);
	}

	activeList = AllocateChunkBuffers(ACTIVE_GRID_SIZE*ACTIVE_GRID_SIZE, 200*1000, 100*1000);

	SetCamPos((vec3){0.0f, 128, 0.0f});
	getCameraTargetAndPosition(&lastPlayerPos, NULL);
	CheckViewDistance(lastPlayerPos);

	//chunk generation thread
	HANDLE handle;
	handle = CreateThread(0,0, WorldThread, 0, 0, NULL);

	return handle;
}

CRITICAL_SECTION* getActiveListCriticalSection(){
	return &activeListMutex;
}

void DrawChunks()
{		
		for (int x = 0; x < ACTIVE_GRID_SIZE; x++) {
			for (int z = 0; z < ACTIVE_GRID_SIZE; z++) {
				EnterCriticalSection(&activeListMutex);
				if (activeList->BufferList[BUFFER_INDEX(x, z)].inUse) {
					GPUBuffer* buffer = &activeList->BufferList[BUFFER_INDEX(x, z)];
					DrawMesh(buffer->vertexBuffer, buffer->indexBuffer, buffer->indexBufferElements, (vec3){buffer->x*CHUNK_SIZE,0.0f, buffer->z*CHUNK_SIZE});
				}
				LeaveCriticalSection(&activeListMutex);
			}
		}	
}

