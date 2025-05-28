#pragma once
#include <stdint.h>

typedef enum{
	NORTH = 0,
	SOUTH = 1,
	TOP = 2,
	BOTTOM = 3,
	WEST = 4, 
	EAST = 5,
}BLOCKFACE;

typedef struct {
	int textureId;
}Face;

typedef struct{
	wchar_t* name;
	uint16_t BlockId;
	Face faces[6];
}BlockType;

BlockType GetBlockTypeByID(uint16_t id);
