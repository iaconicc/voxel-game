#pragma once
#ifndef _AMD64_
#define _AMD64_
#endif
#include "hashmap.h"
#include <stdbool.h>
#include <synchapi.h>
#include "FIFO.h"
#include "chunk.h"

HANDLE StartWorld();
void GetBlock(Block* block, int x, int y, int z);

void DrawChunks();
CRITICAL_SECTION* getChunkmapMutex();