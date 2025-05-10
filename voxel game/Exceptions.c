#include "Exceptions.h"
#include <strsafe.h>

WCHAR ExceptionMessage[512];

WCHAR* GetExceptionMessage()
{
	return &ExceptionMessage;
}

void formatMsg(WCHAR* file, int line, WCHAR* msg)
{
	HRESULT result = StringCchPrintfW(
		ExceptionMessage,
		512,
		L"FILE: %s\nLINE: %u\nDESC: %s",
		file,
		line,
		msg
	);
}