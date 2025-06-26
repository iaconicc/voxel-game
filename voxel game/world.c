#include "world.h"
#include "App.h"
#include "Camera.h"
#include <processthreadsapi.h>
#include "hashmap.h"
#include "DX3D11.h"
#include <math.h>  // Provides fminf() and fmaxf()

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

#define ViewDistance 8
#define ACTIVE_GRID_SIZE (2 * ViewDistance)

#define MIN_SURFACE_HEIGHT  64.0f
#define MAX_SURFACE_HEIGHT  75.0f

fnl_state noiseStateLow;
fnl_state noiseStateHigh;
fnl_state noiseStateBlend;
fnl_state noiseStateContinental;

double lastPlayerPos[3];

float clamp(float value, float min, float max) {
	return fminf(fmaxf(value, min), max);
}

float lerp(float a, float b, float t) {
	return a + t * (b - a);
}

float smoothstep(float edge0, float edge1, float x) {
	// Scale, bias, and saturate x to 0..1 range
	float t = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
	// Apply cubic Hermite interpolation
	return t * t * (3.0f - 2.0f * t);
}

inline static void GetChunkPosFromAbsolutePos(double pos[3], int* x, int* y, int* z)
{
	*x =(int) floorf(pos[0]/ CHUNK_SIZE);
	*y = (int)floorf(pos[1] / CHUNK_SIZE);
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

inline static void GenerateWorld(double pos[3]) {
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
			double currentPlayerPos[3];
			getCameraWorldPos(currentPlayerPos);
			
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

			lastPlayerPos[0] = currentPlayerPos[0];
			lastPlayerPos[1] = currentPlayerPos[1];
			lastPlayerPos[2] = currentPlayerPos[2];
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

inline static float clampf(float val, float min, float max) {
	return val < min ? min : (val > max ? max : val);
}

inline static float getNoise(float x, float z, float scale, float offset){
	float n = fnlGetNoise2D(&noiseStateLow, x * 0.1f + offset, z * 0.1f + offset);
	float n1 = fnlGetNoise2D(&noiseStateHigh, x * 1.0f + offset, z * 1.0f + offset);

	float b =fnlGetNoise2D(&noiseStateBlend, x * 0.3f + offset, z * 0.3f + offset);
	b = clampf((b + 1.0f) * 0.5f, 0.0f, 1.0f);
	b = powf(b, 2.0f);

	return lerp(n, n1, b);
}

inline static int getHeight(int x, int z, float scale, float offset, int minHeight, int maxHeight){
	float noise = getNoise(x, z, scale, offset);
	float normalized = -1.0f*(powf((noise + 1.0f) * 0.5f, 4));
	int range = maxHeight - minHeight;
	return (int)(normalized * range) + minHeight;
}

void GetBlock(Block* block,int x, int y, int z){

	int SurfaceHeight = getHeight(x, z, 0.5f, 0.0f, 64, 150);

	if (y <= SurfaceHeight) {
		if (y == SurfaceHeight) {
			block->blockID = 2;
		}
		else if (y >= (SurfaceHeight - 5)) {
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
	
	int seed = rand();
	int seed1 = rand();

	noiseStateLow = fnlCreateState();
	noiseStateLow.noise_type = FNL_NOISE_PERLIN;
	noiseStateLow.frequency = 0.05f;
	noiseStateLow.seed = seed;
	noiseStateLow.fractal_type = FNL_FRACTAL_FBM;
	noiseStateLow.gain = 0.5f;
	noiseStateLow.lacunarity = 1.0f;
	noiseStateLow.octaves = 10;

	noiseStateHigh = fnlCreateState();
	noiseStateHigh.noise_type = FNL_NOISE_PERLIN;
	noiseStateHigh.frequency = 0.05f;
	noiseStateHigh.seed = seed1;
	noiseStateHigh.fractal_type = FNL_FRACTAL_FBM;
	noiseStateHigh.gain = 0.5f;
	noiseStateHigh.lacunarity = 1.0f;
	noiseStateHigh.octaves = 10;

	noiseStateBlend = fnlCreateState();
	noiseStateBlend.noise_type = FNL_NOISE_PERLIN;
	noiseStateBlend.frequency = 0.5f;
	noiseStateBlend.seed = seed;
	noiseStateBlend.fractal_type = FNL_FRACTAL_NONE;

	//Create worker threads
	for (int i = 0; i < MAXWORKERTHREADS; i++){
		WorkerThreads[i] = CreateThread(0, 0, ChunkJobWorkerTheads, NULL, 0, NULL);
	}

	activeList = AllocateChunkBuffers(ACTIVE_GRID_SIZE*ACTIVE_GRID_SIZE*ACTIVE_GRID_SIZE, 200*1000, 200*1000);
	ChunksToBeChecked = calloc(ACTIVE_GRID_SIZE*ACTIVE_GRID_SIZE*ACTIVE_GRID_SIZE, sizeof(ChunksCheck));
	InitFIFO(&AvailableSpacesOnActiveList, ACTIVE_GRID_SIZE*ACTIVE_GRID_SIZE*ACTIVE_GRID_SIZE, sizeof(int));

	SetCamWorldPos((vec3){ 0, 100, 0});
	getCameraWorldPos(lastPlayerPos);
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
	double worldPos[3];
	getCameraWorldPos(worldPos);

	for (int i = 0; i < ACTIVE_GRID_SIZE*ACTIVE_GRID_SIZE*ACTIVE_GRID_SIZE; i++)
	{
		EnterCriticalSection(&activeListMutex);
		if (activeList->BufferList[i].inUse && activeList->BufferList[i].vertexBufferInBytes != 0 && activeList->BufferList[i].indexBufferElements != 0) {
			GPUBuffer* buffer = &activeList->BufferList[i];
			DrawMesh(buffer->vertexBuffer, buffer->indexBuffer, buffer->indexBufferElements, (vec3) {(float) buffer->x* CHUNK_SIZE - worldPos[0], (float) buffer->y* CHUNK_SIZE - worldPos[1], (float) buffer->z* CHUNK_SIZE - worldPos[2]});
		}
		LeaveCriticalSection(&activeListMutex);
	}
}

