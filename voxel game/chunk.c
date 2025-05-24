#include "chunk.h"
#include "DX3D11.h"

typedef struct {
	unsigned int index[6];
}face;

const static vertex cubeVertexs[8] = {
	{-0.5f, -0.5f, 0.0f},
	{0.5f, -0.5f, 0.0f},
	{0.5f, 0.5f, 0.0f},
	{-0.5f, 0.5f, 0.0f},
	{-0.5f, -0.5f, 1.0f},
	{0.5f, -0.5f, 1.0f},
	{0.5f, 0.5f, 1.0f},
	{-0.5f, 0.5f, 1.0f},
};

const static face cubeFaces[6] = {
	{0, 3, 1, 1, 3, 2}, //south face
	{5, 6, 4, 4, 6, 7}, //north face
	{3, 7, 2, 2, 7, 6}, //top face
	{1, 5, 0, 0, 5, 4}, //bottom face
	{4, 7, 0, 0, 7, 3}, //west face
	{1, 2, 5, 5, 2, 6}, //east face
};

vertex* vertexlist;
face* facelist;

void createBlock()
{
	vertexlist = malloc(8 * sizeof(vertex));
	facelist = malloc(6 * sizeof(face));
	memcpy_s(vertexlist, 8 * sizeof(vertex), cubeVertexs, sizeof(cubeVertexs));
	memcpy_s(facelist, 6 * sizeof(face), cubeFaces, sizeof(cubeFaces));
	createVertexBufferAndAppendToList(vertexlist, 8 * sizeof(vertex));
	createIndexDataBuffer(facelist, 6 * sizeof(face));
}

void destroyBlock()
{
	free(vertexlist);
	free(facelist);
}