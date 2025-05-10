#pragma once
#include <Windows.h>
#include "ReturnCodes.h"

WCHAR* GetExceptionMessage();
void formatMsg(WCHAR* file, int line, WCHAR* msg);

#define WIDE2(x) L##x
#define WIDE1(x) WIDE2(x)
#define WFILE WIDE1(__FILE__)

#define Exception(type, msg) PostQuitMessage(type); formatMsg(WFILE, __LINE__, msg);



