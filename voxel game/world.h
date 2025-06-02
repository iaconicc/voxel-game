#pragma once
#ifndef _AMD64_
#define _AMD64_
#endif
#include "hashmap.h"
#include <stdbool.h>
#include <synchapi.h>
#include "chunk.h"
#include "FIFO.h"

typedef struct {
	int ThreadID;
	struct hashmap* hash;
	FIFO* ThreadQueue;
	CRITICAL_SECTION* criticalSection;
	int x;
	int z;
}chunkGenData;

HANDLE StartWorld();
void GetBlock(Block* block, int x, int y, int z);

void DrawChunks();
CRITICAL_SECTION* getChunkmapMutex();