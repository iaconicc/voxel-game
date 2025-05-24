#pragma once
#include <stdbool.h>
#include <d3d11.h>
#include <cglm.h>

typedef struct {
	float x;
	float y;
	float z;
}vertex;

typedef struct {
	mat4 transformationMatrix;
	mat4 projectionMatrix;
}MatrixBuffers;

void createVertexBufferAndAppendToList(vertex* vertexArray, int sizeInBytes);
void createIndexDataBuffer(void* indexArray, int sizeInBytes);

void CreateDX3D11DeviceForWindow(HWND hwnd);
void DestroyDX3D11DeviceForWindow();
void EndFrame();