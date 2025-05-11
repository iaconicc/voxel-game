#include "App.h"
#include "window.h"
#include "keyboard.h"

HWND g_hwnd;

void DoFrameLogic()
{
	if (keyIsPressed(VK_SPACE))
	{
		PostQuitMessage(100);
	}
}

int ApplicationStartAndRun(HINSTANCE hinstance, int width, int height, WCHAR* name)
{
	//create a window
	g_hwnd = CreateWindowInstance(hinstance, width, height, name);

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