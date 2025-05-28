#pragma once
#include <cglm.h>

void* LoadTextureAtlas(int* width, int* height);

float GetUvOfOneBlockX();
float GetUvOfOneBlockY();
void GetUvOffsetByTexId(int texId, float* x, float* y);
