#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct{
	uint64_t size;
	uint32_t* data;
}BitField;

//creation and setting of bitfield
bool InitBitField(BitField* bitfield, uint64_t size);
void SetBit(BitField* bitfield, int index);
void UnsetBit(BitField* bitfield, int index);
void ToggleBit(BitField* bitfield, int index);

//reading from bitfield
uint8_t ReadBit(BitField* bitfield, int index);

//destroying and reseting bitfield
void ClearField(BitField* bitfield);
void DestroyBitField(BitField* bitfield);