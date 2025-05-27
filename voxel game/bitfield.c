#include "bitfield.h"
#include <malloc.h>

struct BitField{
	uint64_t size;
	uint32_t* data;
};

bool InitBitField(BitField* bitfield, uint64_t size)
{
	bitfield->data = calloc((size+31)/32,sizeof(uint32_t));
	if (!bitfield->data){
		return false;
	}
	bitfield->size = size;
	return true;
}

void SetBit(BitField* bitfield, int index)
{
	if (index < 0 || index >= bitfield->size) return;
	int byteidex = index / 32;
	int bitindex = index % 32;
	bitfield->data[byteidex] |= (1U << bitindex);

}

void UnsetBit(BitField* bitfield, int index)
{
	if (index < 0 || index >= bitfield->size) return;
	int byteidex = index / 32;
	int bitindex = index % 32;
	bitfield->data[byteidex] &= ~(1U << bitindex);
}

void ToggleBit(BitField* bitfield, int index)
{
	if (index < 0 || index >= bitfield->size) return;
	int byteidex = index / 32;
	int bitindex = index % 32;
	bitfield->data[byteidex] ^= (1 << bitindex);
}

uint8_t ReadBit(BitField* bitfield, int index)
{
	if (index < 0 || index >= bitfield->size) return 0;
	int byteidex = index / 32;
	int bitindex = index % 32;
	return (bitfield->data[byteidex] >> bitindex & 1U);
}

void ClearField(BitField* bitfield)
{
	for (int i = 0; i < (bitfield->size + 31) / 32; i++)
		bitfield->data[i] = 0;
}

void DestroyBitField(BitField* bitfield)
{
	if (bitfield->data){
		free(bitfield->data);
	}
}

