#include "App.h"
#include "window.h"

HWND g_hwnd;

void DoFrameLogic()
{

}

int ApplicationStart(HINSTANCE hinstance)
{
	//create a window
	g_hwnd = CreateWindowInstance(hinstance);

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