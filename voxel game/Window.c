#include "window.h"
#include "keyboard.h"

#define MODULE L"Wnd"
#include "Logger.h"

#include "resource.h"
#include <stdint.h>



LRESULT CALLBACK Direct3DWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

HWND CreateWindowInstance(HINSTANCE hInstance, int width, int height, WCHAR* name)
{
	//creating and registering a window class that should have its own device context this is for direct3D stuff in the future
	WNDCLASSEX wc;
	const WCHAR* className = L"VoxelGameClass";
	ZeroMemory(&wc, sizeof(WNDCLASSEX));
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_OWNDC; //window style set to have own window device context
	wc.lpfnWndProc = Direct3DWindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadImage(hInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 256, 256, 0);
	wc.hCursor = NULL;
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = className;
	wc.hIconSm = LoadImage(hInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 64, 64, 0);
	
	
	if (!RegisterClassExW(&wc)) {
		LogException(RC_WND_EXCEPTION, L"Window class registration error occured while the application was running.");
		return NULL;
	}
	LogDebug(L"Registered window class with name: %s and style: 0x%x", wc.lpszClassName, wc.style);

	//creation of window
	HWND hwnd = CreateWindowEx(
		0,
		className,
		name,
		WS_BORDER | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU,
		CW_USEDEFAULT, CW_USEDEFAULT, width, height,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	if (hwnd == NULL)
	{
		LogException(RC_WND_EXCEPTION, L"Window creation error occurred while the application was running.");
		return NULL;
	}
	LogDebug(L"window created with handle: 0x%x", hwnd);

	ShowWindow(hwnd, SW_SHOW);

	//return the window handle for later uses such as rendering
	return hwnd;
}

/* yes I know this ain't good practice to define two of the same data structs in two diffrent files,
but its the only way I know of that exposes the keyboard's internal functions only to the window
*/
typedef struct
{
	void (*OnKeyPressed)(uint8_t virtualKey);
	void (*OnKeyReleased)(uint8_t virtualKey);
	void (*OnChar)(WCHAR character);
	void (*ClearState)();
}keyboardOps;

keyboardOps* keyboardops;

//window event handler
LRESULT CALLBACK Direct3DWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	//window creation and resource creation 
	case WM_CREATE:
		keyboardops = InitKeyboardModuleAndGetOwnership();
		if (!keyboardops)
		{
			LogException(RC_WND_EXCEPTION, L"wnd failed to get keyboard module");
		}
		break;
	//this section handles keyboard events
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
		if (!(lParam & 0x40000000) || isAutoRepeatEnabled())
		{
			if (keyboardops)
			{
				keyboardops->OnKeyPressed(wParam);
				break;
			}
		}
	case WM_SYSKEYUP:
	case WM_KEYUP:
		if (keyboardops)
		{
			keyboardops->OnKeyReleased(wParam);
		}
		break;	
	case WM_CHAR:
		if (keyboardops)
		{
			keyboardops->OnChar(wParam);
		}
		break;
	case WM_KILLFOCUS: //this makes sure that losing focus doesn't cause undefined behaviour
		if (keyboardops)
		{
			keyboardops->ClearState();
		}
		break;
	//closing of window and destruction of resources that belong to it
	case WM_CLOSE:
		PostQuitMessage(1);
		DestroyKeyboardModuleAndRevokeOwnership(&keyboardops);
		break;
	}

	return DefWindowProc(hWnd, Msg, wParam, lParam);
}

//window message pump, it procceses all windows that belong to the proccess not just a single window
int ProcessMessages()
{
	MSG msg;
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{

		if (msg.message == WM_QUIT)
		{
			return msg.wParam;
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}