#include "FIFO.h"
#include <malloc.h>
#include <memory.h>

struct FIFO
{
	uint64_t size;
	size_t elementsize;
	size_t head;
	size_t tail;
	void* data;
};

void InitFIFO(FIFO** fifo, uint64_t size, size_t elementsize)
{
	*fifo = malloc(sizeof(FIFO));
	if (!*fifo)
		return;
	(*fifo)->data = calloc(size, elementsize);
	if (!(*fifo)->data)
	{
		free(*fifo);
		*fifo = NULL;
		return;
	}
	(*fifo)->head = 0;
	(*fifo)->tail = 0;
	(*fifo)->size = size;
	(*fifo)->elementsize = elementsize;
}

bool isFIFOFull(FIFO** fifo)
{
	return (((*fifo)->head + 1) % (*fifo)->size) == (*fifo)->tail;
}

bool isFIFOEmpty(FIFO** fifo)
{
	return (*fifo)->head == (*fifo)->tail;
}

void PushElement(FIFO** fifo,const void* value)
{
	if (isFIFOFull(fifo))
		return;

	uint8_t* DataIndex = (uint8_t*)(*fifo)->data + (*fifo)->elementsize * (*fifo)->head;
	memcpy(DataIndex, value, (*fifo)->elementsize);
	(*fifo)->head = ((*fifo)->head + 1) % (*fifo)->size;
}

void* PopElement(FIFO** fifo)
{
	if (isFIFOEmpty(fifo))
		return NULL;

	uint8_t* DataIndex = (uint8_t*)(*fifo)->data + (*fifo)->elementsize * (*fifo)->tail;
	(*fifo)->tail = ((*fifo)->tail + 1) % (*fifo)->size;
	return DataIndex;
}

void FlushFIFO(FIFO** fifo)
{
	(*fifo)->head = 0;
	(*fifo)->tail = 0;
	memset((*fifo)->data, 0, (*fifo)->size * (*fifo)->elementsize);
}

void DestroyFIFO(FIFO** fifo)
{
	if (*fifo)
	{
		free((*fifo)->data);
		free(*fifo);
		*fifo = NULL;
	}
}
