#define MODULE L"Logger"
#include "Logger.h"

#include <stdio.h>
#include <strsafe.h>
#include <time.h>
#include <direct.h>

FILE* log;

errno_t StartLogger()
{
	//ensure logs folder exists
	int err = _wmkdir(L"logs");
	if (!err == EEXIST && err != 0){
		return -1;
	}

	time_t now = time(NULL);
	struct tm local;
	localtime_s(&local, &now);

	WCHAR formatedfilelocation[200];
	StringCchPrintfW(formatedfilelocation, 200, L"logs\\%04d-%02d-%02d_%02d-%02d-%02d-log.txt", local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,local.tm_hour, local.tm_min, local.tm_sec);
	
	errno_t ferr = _wfopen_s(&log, formatedfilelocation, L"w+");
	LogInfo(L"Logger start");

	return ferr;
}

void StopLogger()
{
	if (log)
	{
		fclose(log);
		log = NULL;
	}
}

void __Log(int Level, WCHAR* module, WCHAR* fmt, ...)
{
	if (!log) return;

	time_t now = time(NULL);
	struct tm local;
	localtime_s(&local, &now);

	WCHAR* levelstring = NULL;
	switch (Level)
	{
	case LOG_INFO:
		levelstring = L"INFO";
		break;
	case LOG_WARNING:
		levelstring = L"WARNING";
		break;
	case LOG_DEBUG:
		#ifdef NDEBUG
		return;
		#endif
		levelstring = L"DEBUG";
		break;
	default:
		return;
	}

	WCHAR msg[1024];
	va_list args;
	va_start(args, fmt);
	vswprintf_s(msg, sizeof(msg)/sizeof(WCHAR), fmt, args);
	va_end(args);

	fwprintf(log, L"[%04d-%02d-%02d %02d:%02d:%02d][%s][%s] %s\n",
		local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,
		local.tm_hour, local.tm_min, local.tm_sec, levelstring, module, msg);
	
	fwprintf(stdin, L"[%04d-%02d-%02d %02d:%02d:%02d][%s][%s] %s\n",
		local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,
		local.tm_hour, local.tm_min, local.tm_sec, levelstring, module, msg);

	fflush(log);
}

void __LogException(WCHAR* file, int line,int type, WCHAR* module, WCHAR* fmt, ...)
{
	if (!log) return;

	time_t now = time(NULL);
	struct tm local;
	localtime_s(&local, &now);

	WCHAR* stringtype = NULL;
	switch (type)
	{
	case RC_WND_EXCEPTIOM:
		stringtype = L"wndException";
		break;
	case RC_KBD_EXCEPTIOM:
		stringtype = L"kbdException";
		break;
	default:
		stringtype = L"unknown";
		break;
	}

	WCHAR msg[1024];
	va_list args;
	va_start(args, fmt);
	vswprintf_s(msg, sizeof(msg) / sizeof(WCHAR), fmt, args);
	va_end(args);

	fwprintf(log,L"[%04d-%02d-%02d %02d:%02d:%02d][ERROR][%s] exception type:%s called at file:%s in line:%u with description:%s\n",
		local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,
		local.tm_hour, local.tm_min, local.tm_sec
		,module, stringtype, file, line, msg);
	
	fwprintf(stderr, L"[%04d-%02d-%02d %02d:%02d:%02d][ERROR][%s] exception type:%s called at file:%s in line:%u with description:%s\n",
		local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,
		local.tm_hour, local.tm_min, local.tm_sec
		, module, stringtype, file, line, msg);

	fflush(log);
}
