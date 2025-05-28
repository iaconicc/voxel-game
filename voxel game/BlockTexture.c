#include "BlockTexture.h"
#include "TextureLoader.h"

#define MODULE L"BlockTexManager"
#include "Logger.h"

WCHAR* texturelocation = L"textures\\Blocks.png";

#define BlockTextureSize 16

int AtlasWidth = 0;
int AtlasHeight = 0;

int AtlasWidthInBlocks = 0;
int AtlasHeightInBlocks = 0;

float AtlasOneBlockInUvX = 0;
float AtlasOneBlockInUvY = 0;

void* LoadTextureAtlas(int* width, int* height){
	void* bitmapArray = NULL;
	if (!(bitmapArray = LoadTextureFromFile(texturelocation, width, height)))
	{
		LogException(L"Failed to load texture atlas at location %s", texturelocation);
		return NULL;
	}

	AtlasWidth = (*width);
	AtlasHeight = (*height);

	AtlasWidthInBlocks = AtlasWidth / BlockTextureSize;
	AtlasHeightInBlocks = AtlasHeight / BlockTextureSize;

	AtlasOneBlockInUvX = 1.0f / AtlasWidthInBlocks;
	AtlasOneBlockInUvY = 1.0f / AtlasHeightInBlocks;

	return bitmapArray;
}

float GetUvOfOneBlockX()
{
	return AtlasOneBlockInUvX;
}

float GetUvOfOneBlockY()
{
	return AtlasOneBlockInUvY;
}

void GetUvOffsetByTexId(int texId, float* x, float* y)
{
	int AtlasX = texId % AtlasWidthInBlocks;
	int AtlasY = texId / AtlasWidthInBlocks;

	*x = AtlasX * AtlasOneBlockInUvX;
	*y = AtlasY * AtlasOneBlockInUvY;
}