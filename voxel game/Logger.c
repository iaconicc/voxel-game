#define MODULE L"Logger"
#include "Logger.h"
#include <stdio.h>
#include <strsafe.h>
#include <time.h>
#include <direct.h>
#include <fcntl.h>
#include <io.h>

FILE* gamelog;
IDXGIInfoQueue* infoManager;
IDXGIDebug* debug;
int currentDXmessage = 0;

IDXGIDebug* getDxgiDebug(){
	return debug;
}

void setupInfoManager(){  
    // Function definition to load from DLL  
    typedef HRESULT (WINAPI* DXGIGetDebugInterface)(REFIID, void**);  

    HMODULE lib = LoadLibraryEx(L"Dxgidebug.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!lib)  
    {  
        LogWarning(L"could not load Dxgidebug.dll");  
        return;  
    }  
    // Get DXGIGetDebugInterface in the DLL  
    DXGIGetDebugInterface DXGIgetdebuginterface = (DXGIGetDebugInterface)GetProcAddress(lib, "DXGIGetDebugInterface");  
    if (!DXGIgetdebuginterface)  
    {  
        LogWarning(L"could not load function: DXGIGetDebugInterface");  
        return;  
    }  

    // Use the function pointer to call the method  
    HRESULT hr = DXGIgetdebuginterface(&IID_IDXGIInfoQueue, (void**)&infoManager);
    if (FAILED(hr))  {  
        LogWarning(L"Failed to get IDXGIInfoQueue interface: %s", formatWin32ErrorCodes(hr));  
    }  

	hr = DXGIgetdebuginterface(&IID_IDXGIDebug, (void**)&debug);
	if (FAILED(hr)) {
		LogWarning(L"Failed to get IDXGIInfoQueue interface: %s", formatWin32ErrorCodes(hr));
	}

	FreeLibrary(lib); // Unload the library
	LogDebug(L"initialised DX info manager");
}

errno_t StartLogger(){  
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

static void stopInfoManager()
{
	if (debug)
	{
		debug->lpVtbl->ReportLiveObjects(debug, DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
		debug->lpVtbl->Release(debug);
	}
	if (infoManager)
	{
		infoManager->lpVtbl->Release(infoManager);
	}
}

void StopLogger()
{
	stopInfoManager();

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

void logDXMessages()
{
	int end = infoManager->lpVtbl->GetNumStoredMessages(infoManager, DXGI_DEBUG_ALL);
	for (int i = currentDXmessage; i < end; i++)
	{
		//get message length in bytes
		size_t messageLength;
		infoManager->lpVtbl->GetMessageW(infoManager, DXGI_DEBUG_ALL, i, NULL, &messageLength);
		//allocate memmory for message and get the message
		DXGI_INFO_QUEUE_MESSAGE* message = (DXGI_INFO_QUEUE_MESSAGE*)malloc(messageLength);
		if (!message)
			break;
		infoManager->lpVtbl->GetMessage(infoManager, DXGI_DEBUG_ALL, i, message, &messageLength);

		//convert message to wchar
		WCHAR* wideDescription = (WCHAR*)malloc(messageLength * sizeof(WCHAR));
		if (!wideDescription)
		{
			free(message);
			break;
		}
		MultiByteToWideChar(CP_UTF8, 0, message->pDescription, -1, wideDescription, messageLength);

		//log message
		__Log(LOG_DEBUG, L"DXGI", L"%s", wideDescription);
		free(message);
		free(wideDescription);
	}
	currentDXmessage = end;
}

void __LogException(WCHAR* file, int line, WCHAR* module, WCHAR* fmt, ...)
{
	if (!gamelog) return;

	time_t now = time(NULL);
	struct tm local;
	localtime_s(&local, &now);

	WCHAR msg[1024];
	va_list args;
	va_start(args, fmt);
	vswprintf_s(msg, sizeof(msg) / sizeof(WCHAR), fmt, args);
	va_end(args);

	WCHAR formattedMsg[2048];

#ifdef _DEBUG //DEBUG
	swprintf_s(formattedMsg, sizeof(formattedMsg) / sizeof(WCHAR),
		L"[%04d-%02d-%02d %02d:%02d:%02d][ERROR][%s] exception called at file:%s in line:%u with description:%s\n",
		local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,
		local.tm_hour, local.tm_min, local.tm_sec,
		module, file, line, msg);
#else // Release: removes file and line
	swprintf_s(formattedMsg, sizeof(formattedMsg) / sizeof(WCHAR),
		L"[%04d-%02d-%02d %02d:%02d:%02d][ERROR][%s] exception called with description:%s\n",
		local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,
		local.tm_hour, local.tm_min, local.tm_sec,
		module, msg);
#endif 

	// Write to log file
	fwprintf(gamelog, L"%s", formattedMsg);

	// Write to console
	fwprintf(stderr, L"%s", formattedMsg);

	// Write to Visual Studio Output window
	OutputDebugStringW(formattedMsg);

	fflush(gamelog);
}

WCHAR* formatWin32ErrorCodes(int hr)
{
	WCHAR* msgbuffer = NULL;
	uint16_t nMsgLen = FormatMessage(
	FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), &msgbuffer, 0, NULL);
	if (nMsgLen == 0)
	{
		return L"unknown error";
	}

	// Allocate a new buffer and copy the message
	WCHAR* msg = (WCHAR*)malloc((nMsgLen + 1) * sizeof(WCHAR));
	if (msg)
	{
		wcscpy_s(msg, nMsgLen + 1, msgbuffer);
	}
	LocalFree(msgbuffer);
	return msg;
}