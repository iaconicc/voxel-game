#pragma once
#include <d3d11_4.h>
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

ID3D11Buffer* createIndexDataBuffer(int* indexArray, int sizeInBytes);
ID3D11Buffer* createVertexBuffer(vertex* vertexArray, int sizeInBytes);

void DrawMesh(ID3D11Buffer* vertexBuffer, ID3D11Buffer* indexBuffer, int indexBufferElements, vec3 pos);

bool DxsettingUp();
void CreateDX3D11DeviceForWindow(HWND hwnd, int width, int height);
void setInactive();
void setactive();
void DestroyDX3D11DeviceForWindow();
void EndFrame();