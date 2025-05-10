#include "window.h"
#include "Exceptions.h"

LRESULT CALLBACK Direct3DWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

HWND CreateWindowInstance(HINSTANCE hInstance)
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
	wc.hIcon = NULL;
	wc.hCursor = NULL;
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = className;
	wc.hIconSm = NULL;
	RegisterClassExW(&wc);

	HWND hwnd = CreateWindowEx(
		0,
		className,
		L"Voxel Game",
		WS_BORDER | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU,
		CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	if (hwnd == NULL)
	{
		Exception(RC_WNDException, L"The window failed to create");
		return NULL;
	}

	ShowWindow(hwnd, SW_SHOW);

	return hwnd;
}

LRESULT CALLBACK Direct3DWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_CLOSE:
		PostQuitMessage(1);
		break;
	}

	DefWindowProc(hWnd, Msg, wParam, lParam);
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