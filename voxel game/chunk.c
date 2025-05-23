#include "chunk.h"
#include "DX3D11.h"

typedef struct {
	int index[6];
}face;

static vertex cubeVertexs[8] = {
	{0.0f, 0.0f, 0.0f},
	{1.0f, 0.0f, 0.0f},
	{1.0f, 1.0f, 0.0f},
	{0.0f, 1.0f, 0.0f},
	{0.0f, 0.0f, 1.0f},
	{1.0f, 0.0f, 1.0f},
	{1.0f, 1.0f, 1.0f},
	{0.0f, 1.0f, 1.0f},
};

static face cubeFaces[6] = {
	{0, 3, 1, 1, 3, 2}, //back face
	{5, 6, 4, 4, 6, 7}, //front face
	{3, 7, 2, 2, 7, 6}, //top face
	{1, 5, 0, 0, 5, 4}, //bottom face
	{4, 7, 0, 0, 7, 3}, //Left face
	{1, 2, 5, 5, 2, 6}, //Right face
};

void createBlock()
{
	vertex* vertexlist = malloc(8 * sizeof(vertex));
	createVertexBufferAndAppendToList(vertexlist, 8 * sizeof(vertex));
}