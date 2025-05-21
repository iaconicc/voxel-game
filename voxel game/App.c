#include "App.h"
#include "window.h"
#include "keyboard.h"

#define MODULE L"APP"
#include "Logger.h"

HWND g_hwnd;

static void DoFrameLogic()
{

}

int ApplicationStartAndRun(int width, int height, WCHAR* name)
{
	//create a window
	g_hwnd = CreateWindowInstance(width, height, name);
	if (!g_hwnd){
		LogWarning(L"The application failed to create window");
		return RC_WND_EXCEPTION;
	}
	LogInfo(L"created window with dimesions %u x %u", width, height);
	
	//start application loop
	LogInfo(L"Starting App loop...");
	while (TRUE)
	{
		int ecode;
		if (ecode = ProcessMessages())
		{
			g_hwnd = NULL;
			CleanupWindow();
			return ecode;
		}
		//game logic
		DoFrameLogic();
	}

}