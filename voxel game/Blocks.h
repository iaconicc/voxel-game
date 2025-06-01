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
	uint16_t textureId;
	uint16_t textureRotation;
}Face;

typedef struct{
	Face faces[6];
}Model;

typedef struct{
	wchar_t* name;
	uint16_t BlockId;
	Model directionalModels[6];
}BlockType;

BlockType GetBlockTypeByID(uint16_t id);
