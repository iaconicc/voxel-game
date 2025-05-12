#pragma once
#include <Windows.h>
#include <stdarg.h>
#include "ReturnCodes.h"

//all files that include this must #define MODULE L"modulename" before including this header

typedef enum {
	LOG_INFO = 0x00,
	LOG_DEBUG = 0x01,
	LOG_WARNING = 0x02,
};

errno_t StartLogger();
void StopLogger();

//please do no use these functions directly use the macros provided below
void __LogException(WCHAR* file, int line, int type, WCHAR* module, WCHAR* fmt, ...);
void __Log(int Level, WCHAR* module, WCHAR* fmt, ...);

//some util macros for expanding __FILE__ into a wide string
#define WIDE2(x) L##x
#define WIDE1(x) WIDE2(x)
#define WFILE WIDE1(__FILE__)

//use these logging macros please
#define LogInfo(fmt, ...) __Log(LOG_INFO, MODULE, fmt, __VA_ARGS__);
#define LogDebug(fmt, ...) __Log(LOG_DEBUG, MODULE, fmt, __VA_ARGS__);
#define LogWarning(fmt, ...) __Log(LOG_WARNING, MODULE, fmt, __VA_ARGS__);
#define LogException(type, fmt, ...) PostQuitMessage(type); __LogException(WFILE, __LINE__, type, MODULE, fmt, __VA_ARGS__);



