#include "Exceptions.h"

WCHAR ExceptionMessage[512];

WCHAR* GetExceptionMessage()
{
	return &ExceptionMessage;
}

void formatMsg(char* file, int line, WCHAR* msg)
{
	//converting file name to wchar
	int convertedchars = 0;
	const int newsize = strlen(file);
	WCHAR filewchar[128];
	mbstowcs_s(&convertedchars, filewchar, newsize, file, 128);
	
	lstrcpyW(ExceptionMessage, L"FILE: ");
	//lstrcpyW(ExceptionMessage + 6, filewchar);
}