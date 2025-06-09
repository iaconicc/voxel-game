#pragma once
#include <d3d11_4.h>
#include <stdbool.h>
#include <d3d11.h>
#include <cglm.h>
#include <stdint.h>

typedef struct {
	vec3 pos;
	vec2 texPos;
}vertex;

void UpdateOnResize(int width, int height);
void toggleFullScreen();

ID3D11Buffer* createIndexDataBuffer(int* indexArray, int sizeInBytes);
ID3D11Buffer* createVertexBuffer(vertex* vertexArray, int sizeInBytes);
ID3D11Buffer** AllocateBuffers(int BufferCount, int min);

void DrawMesh(ID3D11Buffer* vertexBuffer, ID3D11Buffer* indexBuffer, int indexBufferElements, vec3 pos);

bool DxsettingUp();
void CreateDX3D11DeviceForWindow(HWND hwnd, int width, int height);
void setInactive();
void setactive();
void DestroyDX3D11DeviceForWindow();
void EndFrame();
float getFrameDelta();
float getFrameRate();