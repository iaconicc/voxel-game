#pragma once
#include <stdbool.h>
#include <d3d11.h>

typedef struct {
	float x;
	float y;
	float z;
}vertex;

void createVertexBufferAndAppendToList(vertex* vertexArray, int sizeInBytes);
void CreateDX3D11DeviceForWindow(HWND hwnd);
void DestroyDX3D11DeviceForWindow();
void EndFrame();