#pragma once
#include <cglm.h>

void* LoadTextureAtlas(int* width, int* height);

float GetUvOfOneBlockX();
float GetUvOfOneBlockY();
void GetUvOffsetByTexId(uint16_t texId, float* x, float* y);
