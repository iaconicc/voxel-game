#pragma once
#ifndef _AMD64_
#define _AMD64_
#endif
#include "hashmap.h"
#include <stdbool.h>
#include <synchapi.h>

typedef struct {
	struct hashmap* hash;
	CRITICAL_SECTION* criticalSection;
	int x;
	int z;
}chunkGenData;

HANDLE StartWorld();
void DrawChunks();
CRITICAL_SECTION* getChunkmapMutex();