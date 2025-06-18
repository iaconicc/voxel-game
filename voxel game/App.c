#include "App.h"
#include "window.h"
#include "keyboard.h"
#include "DX3D11.h"
#include "chunk.h"
#include "Camera.h"
#include "world.h"
#include <strsafe.h>

#define MODULE L"APP"
#include "Logger.h"

HWND g_hwnd;
HANDLE worldThread;

volatile bool running = false;

static void DoFrameLogic()
{
	float delta = getFrameDelta();

	if(keyIsPressed('W'))
		MoveCameraForward(8.917 * delta);

	if (keyIsPressed('S'))
		MoveCameraBack(8.917 * delta);

	if (keyIsPressed('A'))
		StrafeCameraLeft(8.917 * delta);

	if (keyIsPressed('D'))
		StrafeCameraRight(8.917 * delta);

	if (keyIsPressed(VK_UP))
		RotateCam(0, 35.0 * delta);

	if (keyIsPressed(VK_DOWN))
		RotateCam(0.0, -35.0 * delta);

	if (keyIsPressed(VK_LEFT))
		RotateCam(35.0 * delta, 0.0);

	if (keyIsPressed(VK_RIGHT))
		RotateCam(-35.0 * delta, 0.0);

	if (keyIsPressed(VK_F11))
		toggleFullScreen();
}

static void WINAPI FPSThread(){
	while (running){
		Sleep(100);
		double pos[3];
		getCameraWorldPos(pos);
		WCHAR formatedTitle[200];
		StringCchPrintfW(formatedTitle, 200, L"Voxel-Game fps: %.2f pos:%.2f, %.2f, %.2f", getFrameRate(), pos[0], pos[1], pos[2]);
		SetWindowTitle(formatedTitle);
	}
	return 0;
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
	CreateThread(0, 0, FPSThread, NULL, 0, NULL);

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
			DestroyDX3D11DeviceForWindow();
			return ecode;
		}
		DoFrameLogic();
		DrawChunks();
		EnterCriticalSection(getActiveListCriticalSection());
		EndFrame();
		LeaveCriticalSection(getActiveListCriticalSection());

#ifdef _DEBUG
		logDXMessages();
#endif // _DEBUG
	}

}

bool ProgramIsRunning(){
	return running;
}