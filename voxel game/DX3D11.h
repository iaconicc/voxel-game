#pragma once
#include <stdbool.h>
#include <d3d11.h>

void CreateDX3D11DeviceForWindow(HWND hwnd);
void DestroyDX3D11DeviceForWindow();
void EndFrame();