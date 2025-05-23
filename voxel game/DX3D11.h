#pragma once
#include <stdbool.h>
#include <d3d11.h>

typedef struct {
	int x;
	int y;
	int z;
}vertex;

void CreateDX3D11DeviceForWindow(HWND hwnd);
void DestroyDX3D11DeviceForWindow();
void EndFrame();