#pragma once
#include <stdint.h>

typedef struct{
	wchar_t* name;
	uint16_t BlockId;
	int textureId;
}BlockType;

BlockType GetBlockTypeByID(uint16_t id);
