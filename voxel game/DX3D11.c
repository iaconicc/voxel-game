#include "DX3D11.h"

#define MODULE L"DX3D11"
#include "Logger.h"

#pragma comment(lib,"d3d11.lib")

IDXGISwapChain* swapchain = NULL;
IDXGIDevice* device = NULL;
ID3D11DeviceContext* deviceContext = NULL;

DXGI_SWAP_CHAIN_DESC sd = {
	.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
	.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
	.BufferCount = 2,
	.Flags = 0,
	.BufferDesc.Width = 0,
	.BufferDesc.Height = 0,
	.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM,
	.BufferDesc.RefreshRate.Numerator = 0,
	.BufferDesc.RefreshRate.Denominator = 0,
	.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED,
	.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
	.SampleDesc.Count = 1,
	.SampleDesc.Quality = 0,
};

void CreateDX3D11DeviceForWindow(HWND hwnd)
{
	sd.OutputWindow = hwnd;
	sd.Windowed = true;
	
	int DeviceFlags = 0;

#ifdef _DEBUG
	DeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	HRESULT result = D3D11CreateDeviceAndSwapChain(
	NULL,
	D3D_DRIVER_TYPE_HARDWARE,
	NULL, D3D11_CREATE_DEVICE_DEBUG,
	NULL, 0,
	D3D11_SDK_VERSION,	&sd, &swapchain,
	&device, NULL, &deviceContext);

	if (FAILED(result))
	{
		LOGWIN32EXCEPTION(RC_DX3D11_EXPCEPTION, result);
	}

#ifdef _DEBUG
	setupInfoManager();
#endif

	LogInfo(L"DX3D device created succesfully");
}

void DestroyDX3D11DeviceForWindow()
{
	if (swapchain)
	{
		swapchain->lpVtbl->Release(swapchain);
	}

	if (device)
	{
		device->lpVtbl->Release(device);
	}

	if (deviceContext)
	{
		deviceContext->lpVtbl->Release(deviceContext);
	}
}

void EndFrame()
{
	swapchain->lpVtbl->Present(swapchain, 1u, 0u);
}