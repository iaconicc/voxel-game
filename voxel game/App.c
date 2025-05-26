#include "App.h"
#include "window.h"
#include "keyboard.h"
#include "DX3D11.h"
#include "chunk.h"
#include "Camera.h"

#define MODULE L"APP"
#include "Logger.h"

HWND g_hwnd;

static void DoFrameLogic()
{
	if(keyIsPressed('W'))
		MoveCameraForward(0.1);

	if (keyIsPressed('S'))
		MoveCameraBack(0.1);

	if (keyIsPressed('A'))
		StrafeCameraLeft(0.1);

	if (keyIsPressed('D'))
		StrafeCameraRight(0.1);

	if (keyIsPressed(VK_UP))
		RotateCam(0, 0.5);

	if (keyIsPressed(VK_DOWN))
		RotateCam(0.0, -0.5);

	if (keyIsPressed(VK_LEFT))
		RotateCam(0.5, 0.0);

	if (keyIsPressed(VK_RIGHT))
		RotateCam(-0.5, 0.0);

	if (keyIsPressed(VK_F11))
		toggleFullScreen();
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

	initialiseCamera();
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
		}
		DoFrameLogic();
		EndFrame();
#ifdef _DEBUG
		logDXMessages();
#endif // _DEBUG

	}

}