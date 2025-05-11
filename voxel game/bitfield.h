#pragma once
#include <stdint.h>

typedef struct BitField BitField;

//creation and setting of bitfield
void InitBitField(BitField** bitfield, uint64_t size);
void SetBit(BitField** bitfield, int index);
void UnsetBit(BitField** bitfield, int index);
void ToggleBit(BitField** bitfield, int index);

//reading from bitfield
uint8_t ReadBit(BitField** bitfield, int index);

//destroying and reseting bitfield
void ClearField(BitField** bitfield);
void DestroyBitField(BitField** bitfield);