#include "window.h"
#include "keyboard.h"
#include "Exceptions.h"
#include "resource.h"
#include <stdint.h>

LRESULT CALLBACK Direct3DWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

HWND CreateWindowInstance(HINSTANCE hInstance, int width, int height, WCHAR* name)
{
	WNDCLASSEX wc;
	const WCHAR* className = L"VoxelGameClass";
	ZeroMemory(&wc, sizeof(WNDCLASSEX));
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_OWNDC;
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
		LogException(RC_WND_EXCEPTIOM, L"Window class registration error occured while the application was running.");
		return NULL;
	}

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
		LogException(RC_WND_EXCEPTIOM, L"Window creation error occurred while the application was running.");
		return NULL;
	}

	ShowWindow(hwnd, SW_SHOW);

	return hwnd;
}

typedef struct
{
	void (*OnKeyPressed)(uint8_t virtualKey);
	void (*OnKeyReleased)(uint8_t virtualKey);
	void (*OnChar)(WCHAR character);
	void (*ClearState)();
}keyboardOps;

keyboardOps* keyboardops;

LRESULT CALLBACK Direct3DWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_CREATE:
		keyboardops = InitKeyboardModuleAndGetOwnership();
		break;
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
	case WM_KILLFOCUS:
		if (keyboardops)
		{
			keyboardops->ClearState();
		}
	case WM_CLOSE:
		PostQuitMessage(1);
		DestroyKeyboardModuleAndRevokeOwnership();
		break;
	}

	return DefWindowProc(hWnd, Msg, wParam, lParam);
}

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