#pragma once
#include <windows.h>

HWND CreateWindowInstance(int width, int height, WCHAR* name);
int ProcessMessages();
void CleanupWindow();