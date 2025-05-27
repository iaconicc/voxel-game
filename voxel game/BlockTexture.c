#include "BlockTexture.h"
#include "TextureLoader.h"

#define MODULE L"BlockTexManager"
#include "Logger.h"

WCHAR* texturelocation = L"textures\\Blocks.png";

void* LoadTextureAtlas(int* width, int* height){
	void* bitmapArray;
	if (!(bitmapArray = LoadTextureFromFile(texturelocation, width, height)))
	{
		LogException(L"Failed to load texture atlas at location %s", texturelocation);
		return NULL;
	}
	return bitmapArray;
}