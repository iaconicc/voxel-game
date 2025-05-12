#include "App.h"
#include "window.h"
#include "keyboard.h"

#define MODULE L"APP"
#include "Logger.h"

HWND g_hwnd;

static void DoFrameLogic()
{

}

int ApplicationStartAndRun(HINSTANCE hinstance, int width, int height, WCHAR* name)
{
	//create a window
	g_hwnd = CreateWindowInstance(hinstance, width, height, name);
	LogInfo(L"created window with dimesions %uX%u", width, height);
	
	LogInfo(L"Starting App loop");
	while (TRUE)
	{
		int ecode;
		if (ecode = ProcessMessages())
		{
			return ecode;
		}
		DoFrameLogic();
	}

}