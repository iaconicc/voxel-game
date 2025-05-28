#include "Blocks.h"

BlockType blocktypes[3] = {
	{L"Stone", 0, {0, 0, 0, 0, 0, 0}},
	{L"dirt", 1, {1, 1, 1, 1, 1, 1}},
	{L"grass", 1, {2, 2, 7, 1, 2, 2}},
};

BlockType GetBlockTypeByID(uint16_t id){
	return blocktypes[id];
}
