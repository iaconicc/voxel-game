#pragma once
#include <Windows.h>
#include "ReturnCodes.h"

WCHAR* GetExceptionMessage();
void formatMsg(char* file, int line, WCHAR* msg);

#define Exception(type, msg) PostQuitMessage(type); formatMsg(__FILE__,__LINE__, msg);



