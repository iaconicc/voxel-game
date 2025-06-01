#include "App.h"
#include "window.h"
#include "keyboard.h"
#include "DX3D11.h"
#include "chunk.h"
#include "Camera.h"
#include "world.h"

#define MODULE L"APP"
#include "Logger.h"

HWND g_hwnd;
HANDLE worldThread;

bool running = false;

static void DoFrameLogic()
{
	float delta = getFrameDelta();

	if(keyIsPressed('W'))
		MoveCameraForward(4.317 * delta);

	if (keyIsPressed('S'))
		MoveCameraBack(4.317 * delta);

	if (keyIsPressed('A'))
		StrafeCameraLeft(4.317 * delta);

	if (keyIsPressed('D'))
		StrafeCameraRight(4.317 * delta);

	if (keyIsPressed(VK_UP))
		RotateCam(0, 25.0 * delta);

	if (keyIsPressed(VK_DOWN))
		RotateCam(0.0, -25.0 * delta);

	if (keyIsPressed(VK_LEFT))
		RotateCam(25.0 * delta, 0.0);

	if (keyIsPressed(VK_RIGHT))
		RotateCam(-25.0 * delta, 0.0);

	if (keyIsPressed(VK_F11))
		toggleFullScreen();
}

int ApplicationStartAndRun(int width, int height, WCHAR* name)
{
	//create a window
	g_hwnd = CreateWindowInstance(width, height, name);
	if (!g_hwnd){
		LogWarning(L"The application failed to create window");
		return -1;
	}
	LogInfo(L"created window with dimesions %u x %u", width, height);

	running = true;

	initialiseCamera();
	worldThread = StartWorld();

	//start application loop
	LogInfo(L"Starting App loop...");
	while (TRUE)
	{
		int ecode;
		if (ecode = ProcessMessages())
		{
			g_hwnd = NULL;
			running = false;
			CleanupWindow();
			WaitForSingleObject(worldThread, INFINITE);
			return ecode;
		}
		DoFrameLogic();
		DrawChunks();
		EndFrame();
#ifdef _DEBUG
		logDXMessages();
#endif // _DEBUG

	}

}

bool ProgramIsRunning(){
	return running;
}