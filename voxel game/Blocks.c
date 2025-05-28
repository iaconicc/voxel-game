#include "Blocks.h"

BlockType blocktypes[2] = {
	{L"Stone", 0, 0},
	{L"dirt", 1, 4},
};

BlockType GetBlockTypeByID(uint16_t id){
	return blocktypes[id];
}
