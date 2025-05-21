#include "DX3D11.h"

#define MODULE L"DX3D11"
#include "Logger.h"

#pragma comment(lib,"d3d11.lib")

IDXGISwapChain* swapchain = NULL;
IDXGIDevice* device = NULL;
ID3D11DeviceContext* deviceContext = NULL;


void CreateDX3D11DeviceForWindow(HWND hwnd)
{
	DXGI_SWAP_CHAIN_DESC sd = {0};
	sd.OutputWindow = hwnd;
	sd.Windowed = true;
	sd.BufferCount = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	sd.Flags = 0;
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 0;
	sd.BufferDesc.RefreshRate.Denominator = 0;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	
	
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

	logDXMessages();
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
	swapchain->lpVtbl->Present(swapchain,1u, 0u);
}