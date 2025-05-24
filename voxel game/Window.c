#include "window.h"
#include "keyboard.h"
#include "mouse.h"
#include "DX3D11.h"

#define MODULE L"Wnd"
#include "Logger.h"

#include "resource.h"
#include <stdint.h>

LRESULT CALLBACK Direct3DWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
const WCHAR* className = L"VoxelGameClass";
HINSTANCE wndInstance;

int width = 0;
int height = 0;

int getWindowheight() { return height;}
int getWindowWidth() { return width;}

HWND CreateWindowInstance(int width, int height, WCHAR* name)
{
	wndInstance = GetModuleHandle(NULL);

	width = width;
	height = height;

	//creating and registering a window class that should have its own device context this is for direct3D stuff in the future
	WNDCLASSEX wc = {0};
	
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_OWNDC; //window style set to have own window device context
	wc.lpfnWndProc = Direct3DWindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = wndInstance;
	wc.hIcon = LoadImage(wndInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 256, 256, 0);
	wc.hCursor = NULL;
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = className;
	wc.hIconSm = LoadImage(wndInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 64, 64, 0);
	
	
	if (!RegisterClassExW(&wc)) {
		LogException(RC_WND_EXCEPTION, L"Window class registration error occured while the application was running.");
		return NULL;
	}
	LogDebug(L"Registered window class with name: %s and style: 0x%x", wc.lpszClassName, wc.style);

	//calculate window size based on desired client window size
	uint16_t wndStyle = WS_BORDER | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_MAXIMIZEBOX | WS_SIZEBOX;
	RECT wr = {0};
	wr.left = 100;
	wr.right = width + wr.left;
	wr.top = 100;
	wr.bottom = height + wr.top;
	AdjustWindowRect(&wr, WS_BORDER | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_MAXIMIZEBOX | WS_SIZEBOX, FALSE);
	//creation of window
	HWND hwnd = CreateWindowEx(
		0,
		className,
		name,
		WS_BORDER | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_MAXIMIZEBOX | WS_SIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, 
		wr.right - wr.left, wr.bottom - wr.top,
		NULL,
		NULL,
		wndInstance,
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

keyboardOps* keyboardops = NULL;
MouseOps* mouseOps = NULL;

//window event handler
LRESULT CALLBACK Direct3DWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
		//window creation and resource creation 
	case WM_CREATE:
		if (!InitKeyboardModuleAndGetOwnership(&keyboardops)){
			LogException(RC_WND_EXCEPTION, L"wnd failed to get keyboard module");
		}
		if (!InitMouseModuleAndGetOwnership(&mouseOps)){
			LogException(RC_MOUSE_EXCEPTION, L"wnd failed to get mouse module");
		}
		CreateDX3D11DeviceForWindow(hWnd);
		break;
	//this section handles keyboard events
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
		if (keyboardops)
		{
			if (!(lParam & 0x40000000) || isAutoRepeatEnabled())
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
			break;
		}
	case WM_CHAR:
		if (keyboardops)
		{
			keyboardops->OnChar(wParam);
		}
		break;
	//mouse handling
	case WM_MOUSEMOVE:
	{
		if (mouseOps)
		{
			POINTS pt = MAKEPOINTS(lParam);
			mouseOps->OnMouseMove(pt.x, pt.y);
		}
		break;
	}
	case WM_LBUTTONDOWN:
	{
		if (mouseOps)
		{
			POINTS pt = MAKEPOINTS(lParam);
			mouseOps->OnLeftPressed(pt.x, pt.y);
		}
		break;
	}
	case WM_RBUTTONDOWN:
	{
		if (mouseOps)
		{
			POINTS pt = MAKEPOINTS(lParam);
			mouseOps->OnRightPressed(pt.x, pt.y);
		}
		break;
	}
	case WM_RBUTTONUP:
	{
		if (mouseOps)
		{
			POINTS pt = MAKEPOINTS(lParam);
			mouseOps->OnRightReleased(pt.x, pt.y);
		}
		break;
	}
	case WM_LBUTTONUP:
	{
		if (mouseOps)
		{
			POINTS pt = MAKEPOINTS(lParam);
			mouseOps->OnLeftReleased(pt.x, pt.y);
		}
		break;
	}
	case WM_MOUSEWHEEL:
	{
		if (mouseOps)
		{
			POINTS pt = MAKEPOINTS(lParam);
			if (GET_WHEEL_DELTA_WPARAM(wParam) > 0) {
				mouseOps->OnWheelUp(pt.x, pt.y);
			}
			else if (GET_WHEEL_DELTA_WPARAM(wParam) < 0) {
				mouseOps->OnWheelDown(pt.x, pt.y);
			}
		}
		break;
	}
	case WM_MBUTTONDOWN:
	{
		if (mouseOps)
		{
			POINTS pt = MAKEPOINTS(lParam);
			mouseOps->OnMiddlePressed(pt.x, pt.y);
		}
		break;
	}
	case WM_MBUTTONUP:
	{
		if (mouseOps)
		{
			POINTS pt = MAKEPOINTS(lParam);
			mouseOps->OnMiddleReleased(pt.x, pt.y);
			break;
		}
	}
	case WM_KILLFOCUS: //this makes sure that losing focus doesn't cause undefined behaviour
		if (keyboardops)
		{
			keyboardops->ClearState();
		}
		if (mouseOps)
		{
			mouseOps->ClearState();
		}
		break;
	//respond to resizing
	case WM_SIZE:
	{
		uint16_t width = LOWORD(lParam);
		uint16_t height = HIWORD(lParam);
		UpdateOnResize((int)width, (int) height);
		break;
	}
	//closing of window and destruction of resources that belong to it
	case WM_CLOSE:
		PostQuitMessage(1);
		DestroyKeyboardModuleAndRevokeOwnership(&keyboardops);
		DestroyMouseModuleAndRevokeOwnership(&mouseOps);
		DestroyDX3D11DeviceForWindow();
		DestroyWindow(hWnd);
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
			DestroyKeyboardModuleAndRevokeOwnership(&keyboardops);
			DestroyMouseModuleAndRevokeOwnership(&mouseOps);
			DestroyDX3D11DeviceForWindow();
			return msg.wParam;
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

void CleanupWindow()
{
	UnregisterClass(className, wndInstance);
}