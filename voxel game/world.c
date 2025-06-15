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
	int y;
	int z;
	bool Active;
	bool Adding;
}ChunksCheck;

ChunksCheck* ChunksToBeChecked;
FIFO* AvailableSpacesOnActiveList;

typedef (*JobFunction)(void* data);

typedef struct {
	JobFunction func;
	void* data;
}ChunkJob;

#define MAXWORKERTHREADS 8
#define MAXJOBS 32000
HANDLE WorkerThreads[MAXWORKERTHREADS];
FIFO* JobQueue = NULL;

CONDITION_VARIABLE WorkerSleepCondition;
CRITICAL_SECTION JobQueueMutex;

bool ThreadPoolRunning = true;

#define ViewDistance 4
#define ACTIVE_GRID_SIZE (2 * ViewDistance)

#define MIN_SURFACE_HEIGHT  1.0f
#define MAX_SURFACE_HEIGHT  128.0f

fnl_state noise;

vec3 lastPlayerPos;

inline static void GetChunkPosFromAbsolutePos(vec3 pos, int* x, int* y,int* z)
{
	*x =(int) floorf(pos[0]/ CHUNK_SIZE);
	*y = (int)floorf(pos[2] / CHUNK_SIZE);
	*z =(int) floorf(pos[2] / CHUNK_SIZE);
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
	int Chunkx = 0, chunkY = 0, Chunkz = 0;
	GetChunkPosFromAbsolutePos(pos, &Chunkx, &chunkY,&Chunkz);

	for (int x = Chunkx - ViewDistance; x < Chunkx + ViewDistance; x++) {
		for (int y = chunkY - ViewDistance; y < chunkY + ViewDistance; y++) {
			for (int z = Chunkz - ViewDistance; z < Chunkz + ViewDistance; z++) {
				chunkGenData* genData = calloc(1, sizeof(chunkGenData));
				genData->criticalSection = &activeListMutex;
				genData->chunkBuffers = activeList;
				genData->x = x;
				genData->y = y;
				genData->z = z;
				genData->ActiveIndex = sizeOfActiveList;
				QueueChunkJob(RunChunkGen, genData);
				sizeOfActiveList++;
			}
		}
	}
}

inline static void CheckViewDistance(int currentChunkPosX, int currentChunkPosY,int currentChunkPosZ){
	
	size_t sizeOfChunksToBeChecked = 0;
	//check for chunks within player view distance
	for (int x = currentChunkPosX - ViewDistance; x < currentChunkPosX + ViewDistance; x++) {
		for (int y = currentChunkPosY - ViewDistance; y < currentChunkPosY + ViewDistance; y++) {
			for (int z = currentChunkPosZ - ViewDistance; z < currentChunkPosZ + ViewDistance; z++) {
				ChunksToBeChecked[sizeOfChunksToBeChecked].x = x;
				ChunksToBeChecked[sizeOfChunksToBeChecked].y = y;
				ChunksToBeChecked[sizeOfChunksToBeChecked].z = z;
				sizeOfChunksToBeChecked++;
			}
		}
	}

	// Iterate over all active grid positions to find available positions
	for (size_t i = 0; i < (ACTIVE_GRID_SIZE * ACTIVE_GRID_SIZE * ACTIVE_GRID_SIZE); i++) {
		bool match = false;

		if (!activeList->BufferList[i].inUse) {
			goto free;
		}

		for (size_t j = 0; j < sizeOfChunksToBeChecked; j++) {

			if (ChunksToBeChecked[j].x == activeList->BufferList[i].x &&
				ChunksToBeChecked[j].y == activeList->BufferList[i].y &&
				ChunksToBeChecked[j].z == activeList->BufferList[i].z) {
				ChunksToBeChecked[j].Active = true;
				match = true;
				break;
			}
		}

		free:
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
			genData->y = ChunksToBeChecked[i].y;
			genData->z = ChunksToBeChecked[i].z;
			int* availablePtr = PopElement(&AvailableSpacesOnActiveList);
			if (!availablePtr){break; free(genData);};
			int available = *availablePtr;
			genData->ActiveIndex = available;
			ChunksToBeChecked->Adding = true;
			QueueChunkJob(RunChunkGen, genData);
		}
	}
	
	int* availablePtr;
	while ((availablePtr = PopElement(&AvailableSpacesOnActiveList))){
		int available = *availablePtr;
		EnterCriticalSection(&activeListMutex);
		activeList->BufferList[available].inUse = false;
		LeaveCriticalSection(&activeListMutex);
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
			int LastChunky = 0;
			int LastChunkz = 0;

			int Chunkx = 0;
			int Chunky = 0;
			int Chunkz = 0;

			GetChunkPosFromAbsolutePos(lastPlayerPos, &LastChunkx, &LastChunky,&LastChunkz);
			GetChunkPosFromAbsolutePos(currentPlayerPos, &Chunkx, &Chunky,&Chunkz);
			
			if (Chunkx != LastChunkx || Chunky != LastChunky || Chunkz != LastChunkz){
				CheckViewDistance(Chunkx, Chunky,Chunkz);
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

#define CLAMP(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

void GetBlock(Block* block,int x, int y, int z){

		block->blockstate = UnsetBLOCKSOLID(block->blockstate);
		if(y == 0){
			block->blockstate = SetBLOCKSOLID(block->blockstate);
			block->blockID = 4;
			return;
		}

		float val = fnlGetNoise2D(&noise, x, z);
		int Surfaceheight = (int)(((val + 1.0f) * 0.5f) * (MAX_SURFACE_HEIGHT - MIN_SURFACE_HEIGHT) + MIN_SURFACE_HEIGHT);
		//int Surfaceheight =  64;

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

	activeList = AllocateChunkBuffers(ACTIVE_GRID_SIZE*ACTIVE_GRID_SIZE*ACTIVE_GRID_SIZE, 150*1000, 150*1000);
	ChunksToBeChecked = calloc(ACTIVE_GRID_SIZE*ACTIVE_GRID_SIZE*ACTIVE_GRID_SIZE, sizeof(ChunksCheck));
	InitFIFO(&AvailableSpacesOnActiveList, ACTIVE_GRID_SIZE*ACTIVE_GRID_SIZE*ACTIVE_GRID_SIZE, sizeof(int));

	SetCamPos((vec3){128, 65, 128});
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
	for (int i = 0; i < ACTIVE_GRID_SIZE*ACTIVE_GRID_SIZE*ACTIVE_GRID_SIZE; i++)
	{
		EnterCriticalSection(&activeListMutex);
		if (activeList->BufferList[i].inUse && activeList->BufferList[i].vertexBufferInBytes != 0 && activeList->BufferList[i].indexBufferElements != 0) {
			GPUBuffer* buffer = &activeList->BufferList[i];
			DrawMesh(buffer->vertexBuffer, buffer->indexBuffer, buffer->indexBufferElements, (vec3) { buffer->x* CHUNK_SIZE, buffer->y* CHUNK_SIZE, buffer->z* CHUNK_SIZE});
		}
		LeaveCriticalSection(&activeListMutex);
	}
}

