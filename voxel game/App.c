#include "App.h"
#include "window.h"
#include "keyboard.h"
#include "DX3D11.h"
#include "chunk.h"

#define MODULE L"APP"
#include "Logger.h"

HWND g_hwnd;

static void DoFrameLogic()
{
	EndFrame();
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
	
	createBlock();

	//start application loop
	LogInfo(L"Starting App loop...");
	while (TRUE)
	{
		int ecode;
		if (ecode = ProcessMessages())
		{
			destroyBlock();
			g_hwnd = NULL;
			CleanupWindow();
			return ecode;

#ifdef _DEBUG
			logDXMessages();
#endif // _DEBUG

		}
		//game logic
		DoFrameLogic();

#ifdef _DEBUG
		logDXMessages();
#endif // _DEBUG

	}

}