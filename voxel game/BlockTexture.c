#include "BlockTexture.h"
#include "TextureLoader.h"
#include <cglm.h>

#define MODULE L"BlockTexManager"
#include "Logger.h"

WCHAR* texturelocation = L"textures\\Blocks.png";

#define TEXTURE_SIZE_IN_PIXELS 16
int AtlasWidth = 0;
int AtlasHeight = 0;
int RowSize = 0;
int columnSize = 0;

float OneTextureByTexCoordW = 0.0f;
float OneTextureByTexCoordH = 0.0f;

void* LoadTextureAtlas(int* width, int* height){
	void* bitmapArray;
	if (!(bitmapArray = LoadTextureFromFile(texturelocation, width, height)))
	{
		LogException(L"Failed to load texture atlas at location %s", texturelocation);
		return NULL;
	}
	AtlasWidth = (*width);
	AtlasHeight = (*height);
	RowSize = AtlasWidth / TEXTURE_SIZE_IN_PIXELS;
	columnSize = AtlasHeight / TEXTURE_SIZE_IN_PIXELS;
	OneTextureByTexCoordW = 1.0f / (float) RowSize;
	OneTextureByTexCoordH = 1.0f / (float) columnSize;
	return bitmapArray;
}

float GetTexcoordFromAtlasX(){
	0.25f;
}

float GetTexcoordFromAtlasY(){
	0.25f;
}