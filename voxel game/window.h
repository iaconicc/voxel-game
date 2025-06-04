#pragma once
#include <windows.h>

HWND CreateWindowInstance(int width, int height, WCHAR* name);
int ProcessMessages();
void CleanupWindow();

int getWindowheight();
int getWindowWidth();
void SetWindowTitle(WCHAR* string);