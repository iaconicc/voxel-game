#define MODULE L"Logger"
#include "Logger.h"
#include <stdio.h>
#include <strsafe.h>
#include <time.h>
#include <direct.h>
#include <fcntl.h>
#include <io.h>

FILE* gamelog;



errno_t StartLogger()  
{  
	if (_setmode(_fileno(stdout), _O_U16TEXT) == -1) {
		fwprintf(stderr, L"Failed to set mode for stdout. Falling back to default mode.\n");
		// Optionally, use _O_TEXT or _O_BINARY as a fallback
	}

  // Ensure logs folder exists  
  int err = _wmkdir(L"logs");  
  if (!err == EEXIST && err != 0) {  
      return -1;  //fails if the error wasn't the folder already
  }  

  time_t now = time(NULL);  
  struct tm local;  
  localtime_s(&local, &now);  

  WCHAR formatedfilelocation[200];  
  StringCchPrintfW(formatedfilelocation, 200, L"logs\\%04d-%02d-%02d_%02d-%02d-%02d-log.txt", local.tm_year + 1900, local.tm_mon + 1, local.tm_mday, local.tm_hour, local.tm_min, local.tm_sec);  

  errno_t ferr = _wfopen_s(&gamelog, formatedfilelocation, L"wb+");

  if (ferr != 0 || !gamelog) {
	  fwprintf(stderr, L"Failed to open log file: %d\n", ferr);
	  return ferr;
  }

  if (fputwc(0xFEFF, gamelog) == WEOF) { //making sure there is UTF-16 support
	  fwprintf(stderr, L"Failed to write BOM to log file.\n");
	  return -1;
  }

  else {
	  fwprintf(stderr, L"BOM written successfully.\n");
  }
  fflush(gamelog); // Ensure it is flushed to disk  

  LogInfo(L"Logging has started ƒc");

  return ferr;  
}

void StopLogger()
{
	if (gamelog)
	{
		fclose(gamelog);
		gamelog = NULL;
	}
}

void __Log(int Level, WCHAR* module, WCHAR* fmt, ...)
{
	if (!gamelog) return;

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


	WCHAR formattedMsg[2048];
	swprintf_s(formattedMsg, sizeof(formattedMsg) / sizeof(WCHAR),
		L"[%04d-%02d-%02d %02d:%02d:%02d][%s][%s] %s\n",
		local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,
		local.tm_hour, local.tm_min, local.tm_sec, levelstring, module, msg);

	// Write to log file
	fwprintf(gamelog, L"%s", formattedMsg);

	// Write to console
	fwprintf(stdout, L"%s", formattedMsg);

	// Write to Visual Studio Output window
	OutputDebugStringW(formattedMsg);

	fflush(gamelog);
}

void __LogException(WCHAR* file, int line,int type, WCHAR* module, WCHAR* fmt, ...)
{
	if (!gamelog) return;

	time_t now = time(NULL);
	struct tm local;
	localtime_s(&local, &now);

	WCHAR* stringtype = NULL;
	switch (type)
	{
	case RC_WND_EXCEPTION:
		stringtype = L"wndException";
		break;
	case RC_KBD_EXCEPTION:
		stringtype = L"kbdException";
		break;
	case RC_MOUSE_EXCEPTION:
		stringtype = L"mouseException";
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

	WCHAR formattedMsg[2048];
	swprintf_s(formattedMsg, sizeof(formattedMsg) / sizeof(WCHAR),
		L"[%04d-%02d-%02d %02d:%02d:%02d][ERROR][%s] exception type:%s called at file:%s in line:%u with description:%s\n",
		local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,
		local.tm_hour, local.tm_min, local.tm_sec,
		module, stringtype, file, line, msg);

	// Write to log file
	fwprintf(gamelog, L"%s", formattedMsg);

	// Write to console
	fwprintf(stderr, L"%s", formattedMsg);

	// Write to Visual Studio Output window
	OutputDebugStringW(formattedMsg);

	fflush(gamelog);
}
