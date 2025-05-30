#pragma once
#include <stdbool.h>
#include <d3d11.h>
#include <cglm.h>

typedef struct {
	vec3 pos;
	vec2 texPos;
}vertex;

typedef struct {
	mat4 transformationMatrix;
	mat4 projectionMatrix;
	mat4 viewMatrix;
}MatrixBuffers;

void createVertexBufferAndAppendToList(vec3* vertexArray, int sizeInBytes);
void createIndexDataBuffer(void* indexArray, int sizeInBytes);

void UpdateOnResize(int width, int height);
void toggleFullScreen();

void CreateDX3D11DeviceForWindow(HWND hwnd, int width, int height);
void DestroyDX3D11DeviceForWindow();
void EndFrame();