#pragma once
#include <windows.h>

HWND CreateWindowInstance(HINSTANCE hInstance, int width, int height, WCHAR* name);
int ProcessMessages();