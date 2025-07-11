#pragma once
#include <d3d11_4.h>
#include <stdbool.h>
#include <d3d11.h>
#include <cglm.h>
#include <stdint.h>

typedef struct{
	int8_t x;
	int8_t y;
	int8_t z;
	int8_t w;
	uint16_t texID; // we'll use the upper two bits to encode a local uv as either 1 or 0 for u and v respectivly
	uint8_t texPlane;
	uint8_t rotation;
}vertex;

void UpdateOnResize(int width, int height);
void toggleFullScreen();

ID3D11Buffer* createIndexDataBuffer(int* indexArray, int sizeInBytes);
ID3D11Buffer* createVertexBuffer(vertex* vertexArray, int sizeInBytes);


typedef struct {
	int Rx, Ry;
	int x, y, z;
	int vertexBufferInBytes;
	int indexBufferElements;
	bool inUse;
	CRITICAL_SECTION bufferMutex;
	ID3D11Buffer* vertexBuffer;
	ID3D11Buffer* indexBuffer;
}GPUBuffer;

typedef struct {
	GPUBuffer* BufferList;
	int BufferCount;
	int BufferVertexMinSize;
	int BufferIndexMinSize;
	int totalVertexBufferSizeInBytes;
	int totalIndexBufferSizeInBytes;
}ChunkBuffers;

ChunkBuffers* AllocateChunkBuffers(int BufferCount, int vertexMin, int indexMin);
void updateBuffer(GPUBuffer* buffer, vertex* vertexs, int* indices);
void copyBuffer(ID3D11Buffer* destination, ID3D11Buffer* source);
void ReleaseChunkBuffers(ChunkBuffers* buffers);

void DrawMesh(ID3D11Buffer* vertexBuffer, ID3D11Buffer* indexBuffer, int indexBufferElements, vec3 pos);

bool DxsettingUp();
void CreateDX3D11DeviceForWindow(HWND hwnd, int width, int height);
void setInactive();
void setactive();
void DestroyDX3D11DeviceForWindow();
void EndFrame();
float getFrameDelta();
float getFrameRate();