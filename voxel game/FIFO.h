#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct FIFO FIFO;

void InitFIFO(FIFO** fifo, uint64_t size, size_t elementsize);
bool isFIFOFull(FIFO** fifo);
bool isFIFOEmpty(FIFO** fifo);
void* PopElement(FIFO** fifo);
void PushElement(FIFO** fifo, const void* value);
void FlushFIFO(FIFO** fifo);
void DestroyFIFO(FIFO** fifo);