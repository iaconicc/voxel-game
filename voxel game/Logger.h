#pragma once
#include <Windows.h>
#include <stdarg.h>
#include <stdint.h>
#include "ReturnCodes.h"


//all files that include this must #define MODULE L"modulename" before including this header

typedef enum {
	LOG_INFO = 0x00,
	LOG_DEBUG = 0x01,
	LOG_WARNING = 0x02,
} LOGGINGLEVELS;

errno_t StartLogger();
void StopLogger();

//please do no use these functions directly use the macros provided below
void __LogException(WCHAR* file, int line, int type, WCHAR* module, WCHAR* fmt, ...);
void __Log(int Level, WCHAR* module, WCHAR* fmt, ...);
WCHAR* formatWin32ErrorCodes(int hr);
void logDXMessages();
void setupInfoManager();

//some util macros for expanding __FILE__ into a wide string
#define WIDE2(x) L##x
#define WIDE1(x) WIDE2(x)
#define WFILE WIDE1(__FILE__)

//use these logging macros please
#define LogInfo(fmt, ...) __Log(LOG_INFO, MODULE, fmt, __VA_ARGS__);

#ifdef _DEBUG
#define LogDebug(fmt, ...) __Log(LOG_DEBUG, MODULE, fmt, __VA_ARGS__);
#else
#define LogDebug(fmt, ...) 
#endif

#define LogWarning(fmt, ...) __Log(LOG_WARNING, MODULE, fmt, __VA_ARGS__);
#define LogException(type, fmt, ...) PostQuitMessage(type); __LogException(WFILE, __LINE__, type, MODULE, fmt, __VA_ARGS__);

#define LOGWIN32EXCEPTION(type,hr) PostQuitMessage(type); WCHAR* msg = formatWin32ErrorCodes(hr);  __LogException(WFILE, __LINE__, type, MODULE, msg); free(msg)


