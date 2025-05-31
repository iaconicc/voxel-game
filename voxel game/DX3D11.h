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

void UpdateOnResize(int width, int height);
void toggleFullScreen();

void DrawMesh(vertex* vertexList, int* indexList, int vertexListByteSize, int indexListByteSize, vec3 pos);

bool DxsettingUp();
void CreateDX3D11DeviceForWindow(HWND hwnd, int width, int height);
void setInactive();
void setactive();
void DestroyDX3D11DeviceForWindow();
void EndFrame();