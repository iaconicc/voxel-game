#include "world.h"
#include "App.h"
#include "Camera.h"
#include <processthreadsapi.h>
#include "hashmap.h"
#include "DX3D11.h"

#define FNL_IMPL
#include <FastNoiseLite.h>

#define MODULE L"WORLD"
#include "Logger.h"

CRITICAL_SECTION activeListMutex;
ChunkBuffers* activeList;

typedef struct {
	int x;
	int z;
	bool Active;
}ChunksCheck;

ChunksCheck* ChunksToBeChecked;
FIFO* AvailableSpacesOnActiveList;

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

#define ViewDistance 16
#define ACTIVE_GRID_SIZE (2 * ViewDistance)
#define BUFFER_INDEX(x, z) ((ACTIVE_GRID_SIZE * (x)) + (z))

#define MIN_SURFACE_HEIGHT  64.0f
#define MAX_SURFACE_HEIGHT  126.0f

#define WORLDSIZEINCHUNKS 256
#define WORLDSIZEINBLOCKS WORLDSIZEINCHUNKS * CHUNK_SIZE
fnl_state noise;

vec3 lastPlayerPos;

inline static void GetChunkPosFromAbsolutePos(vec3 pos, int* x, int* z)
{
	*x =(int) floorf(pos[0]/ CHUNK_SIZE);
	*z =(int) floorf(pos[2] / CHUNK_SIZE);
}

inline static bool ChunkIsInWorld(int x, int z) {
	return x >= 0 && x < WORLDSIZEINCHUNKS && z >= 0 && z < WORLDSIZEINCHUNKS;
}

static void RunChunkGen(void* data){
	chunkGenData* ChunkGenData = (chunkGenData*) data;
	generateChunkMesh(ChunkGenData);//the data is freed on generateChunkMesh() please do not double free data
}

inline static void QueueChunkJob(JobFunction func, void* data){
	ChunkJob job = {func, data};

	EnterCriticalSection(&JobQueueMutex);
	PushElement(&JobQueue, &job);
	WakeConditionVariable(&WorkerSleepCondition);
	LeaveCriticalSection(&JobQueueMutex);
}

inline static void GenerateWorld(vec3 pos) {
	size_t sizeOfActiveList = 0;
	int Chunkx = 0, Chunkz = 0;
	GetChunkPosFromAbsolutePos(pos, &Chunkx, &Chunkz);

	for (int x = Chunkx - ViewDistance; x < Chunkx + ViewDistance; x++) {
		for (int z = Chunkz - ViewDistance; z < Chunkz + ViewDistance; z++) {
			if (ChunkIsInWorld(x, z)) {
				chunkGenData* genData = calloc(1, sizeof(chunkGenData));
				genData->criticalSection = &activeListMutex;
				genData->chunkBuffers = activeList;
				genData->x = x;
				genData->z = z;
				genData->ActiveIndex = sizeOfActiveList;
				QueueChunkJob(RunChunkGen, genData);
				sizeOfActiveList++;
			}
		}
	}
}

inline static void CheckViewDistance(int lastChunkPosX, int lastChunkPosZ, int currentChunkPosX, int currentChunkPosZ){
	
	size_t sizeOfChunksToBeChecked = 0;
	//check for chunks within player view distance
	for (int x = currentChunkPosX - ViewDistance; x < currentChunkPosX + ViewDistance; x++) {
		for (int z = currentChunkPosZ - ViewDistance; z < currentChunkPosZ + ViewDistance; z++) {
			if (ChunkIsInWorld(x,z)){
				ChunksToBeChecked[sizeOfChunksToBeChecked].x = x;
				ChunksToBeChecked[sizeOfChunksToBeChecked].z = z;
				sizeOfChunksToBeChecked++;
			}
		}
	}

	// Iterate over all active grid positions to find available positions
	for (size_t i = 0; i < (ACTIVE_GRID_SIZE * ACTIVE_GRID_SIZE); i++) {
		bool match = false;

		for (size_t j = 0; j < sizeOfChunksToBeChecked; j++) {
			if (ChunksToBeChecked[j].x == activeList->BufferList[i].x &&
				ChunksToBeChecked[j].z == activeList->BufferList[i].z) {
				ChunksToBeChecked[j].Active = true;
				match = true;
				break;
			}
		}

		if (!match) {
			PushElement(&AvailableSpacesOnActiveList, &i);
		}
	}

	for (size_t i = 0; i < sizeOfChunksToBeChecked; i++) {
		if(!ChunksToBeChecked[i].Active){
			chunkGenData* genData = calloc(1, sizeof(chunkGenData));
			genData->criticalSection = &activeListMutex;
			genData->chunkBuffers = activeList;
			genData->x = ChunksToBeChecked[i].x;
			genData->z = ChunksToBeChecked[i].z;
			int* availablePtr = PopElement(&AvailableSpacesOnActiveList);
			if (!availablePtr) break;
			int available = *availablePtr;
			genData->ActiveIndex = available;
			QueueChunkJob(RunChunkGen, genData);
		}
	}

	memset(ChunksToBeChecked, 0, sizeOfChunksToBeChecked * sizeof(ChunksCheck));
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
				CheckViewDistance(LastChunkx, LastChunkz, Chunkx, Chunkz);
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
	DestroyFIFO(&AvailableSpacesOnActiveList);
	ReleaseChunkBuffers(activeList);
	free(ChunksToBeChecked);
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

		float val = fnlGetNoise2D(&noise, x, z);
		int Surfaceheight = (int)(((val + 1.0f) * 0.5f) * (MAX_SURFACE_HEIGHT - MIN_SURFACE_HEIGHT) + MIN_SURFACE_HEIGHT);

		if (y <= Surfaceheight) {
			if (y == Surfaceheight){
				block->blockID = 2;
			}
			else if(y >= (Surfaceheight-5)){
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

	noise = fnlCreateState();
	noise.noise_type = FNL_NOISE_PERLIN;
	noise.frequency = 0.01f;
	noise.seed = rand();
	noise.fractal_type = FNL_FRACTAL_FBM;
	noise.octaves = 10;
	noise.lacunarity = 2.0f;
	noise.gain = 0.5f;

	//Create worker threads
	for (int i = 0; i < MAXWORKERTHREADS; i++){
		WorkerThreads[i] = CreateThread(0, 0, ChunkJobWorkerTheads, NULL, 0, NULL);
	}

	activeList = AllocateChunkBuffers(ACTIVE_GRID_SIZE*ACTIVE_GRID_SIZE, 200*1000, 150*1000);
	ChunksToBeChecked = calloc(ACTIVE_GRID_SIZE * ACTIVE_GRID_SIZE, sizeof(ChunksCheck));
	InitFIFO(&AvailableSpacesOnActiveList, ACTIVE_GRID_SIZE * ACTIVE_GRID_SIZE, sizeof(int));

	SetCamPos((vec3){WORLDSIZEINBLOCKS/2, 128, WORLDSIZEINBLOCKS/2});
	getCameraTargetAndPosition(&lastPlayerPos, NULL);
	GenerateWorld(lastPlayerPos);

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

