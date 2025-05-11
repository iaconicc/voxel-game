#pragma once
#include <Windows.h>
#include "ReturnCodes.h"

WCHAR* GetExceptionMessage();
void formatMsg(WCHAR* file, int line, WCHAR* msg);

#define WIDE2(x) L##x
#define WIDE1(x) WIDE2(x)
#define WFILE WIDE1(__FILE__)

#define WSLINE2(x) L#x
#define WSLINE1(x) WSLINE2(x)
#define WSLINE WSLINE1(__LINE__) 

#define Exception(type, msg) PostQuitMessage(type); LogException(WFILE, WSLINE, msg);



