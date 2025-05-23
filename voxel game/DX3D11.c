#include "DX3D11.h"
#include <d3dcompiler.h>

#define MODULE L"DX3D11"
#include "Logger.h"

#pragma comment(lib,"d3d11.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "D3DCompiler.lib")

IDXGISwapChain* swapchain = NULL;
ID3D11Device* device = NULL;
ID3D11DeviceContext* deviceContext = NULL;
ID3D11RenderTargetView* renderTargetView = NULL;

ID3D11Buffer* vertexBuffers[32];
ID3D11Buffer* IndexBuffer = NULL;

ID3D11VertexShader* vertexShader = NULL;
ID3D11PixelShader* pixelShader = NULL;

UINT offset = 0;
UINT VertexSizeInBytes = sizeof(vertex);

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

#ifdef _DEBUG
#define LOGDXMESSAGES() logDXMessages()
#else
#define LOGDXMESSAGES()
#endif

#define DXFUNCTIONFAILED(hrcall) if(FAILED(hr = (hrcall))){ LOGDXMESSAGES(); LOGWIN32EXCEPTION(RC_DX3D11_EXPCEPTION, hr); return;}
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

#ifdef _DEBUG
	setupInfoManager();
#endif

	if (FAILED(result))
	{
		LOGDXMESSAGES();
		LOGWIN32EXCEPTION(RC_DX3D11_EXPCEPTION, result);
		return;
	}

	LogInfo(L"DX3D device created succesfully");

	LogInfo(L"setting up base pipeline...");
	//setup pipeline
	HRESULT hr;

	//getting swap chain buffer
	ID3D11Resource* backBuffer = NULL;
	DXFUNCTIONFAILED(swapchain->lpVtbl->GetBuffer(swapchain, 0, &IID_ID3D11Resource, &backBuffer));
	
	//create render target from back buffer
	DXFUNCTIONFAILED(device->lpVtbl->CreateRenderTargetView(device, backBuffer, NULL, &renderTargetView));
	backBuffer->lpVtbl->Release(backBuffer);

	//bind vertex buffer
	deviceContext->lpVtbl->IASetVertexBuffers(deviceContext, 0, 1, &vertexBuffers, &VertexSizeInBytes, &offset);

	//bind index buffer
	deviceContext->lpVtbl->IASetIndexBuffer(deviceContext, IndexBuffer, DXGI_FORMAT_R16_UINT, 0);
	LogInfo(L"pipeline set");

	//create vertex shader
	ID3DBlob* ShaderBlob = NULL;
	DXFUNCTIONFAILED(D3DReadFileToBlob(L"VertexShader.cso", &ShaderBlob));
	DXFUNCTIONFAILED(device->lpVtbl->CreateVertexShader(device, ShaderBlob->lpVtbl->GetBufferPointer(ShaderBlob), ShaderBlob->lpVtbl->GetBufferSize(ShaderBlob), NULL, &vertexShader));
	ShaderBlob->lpVtbl->Release(ShaderBlob);

	//create pixel shader
	DXFUNCTIONFAILED(D3DReadFileToBlob(L"PixelShader.cso", &ShaderBlob));
	DXFUNCTIONFAILED(device->lpVtbl->CreatePixelShader(device, ShaderBlob->lpVtbl->GetBufferPointer(ShaderBlob), ShaderBlob->lpVtbl->GetBufferSize(ShaderBlob), NULL, &vertexShader));
	ShaderBlob->lpVtbl->Release(ShaderBlob);

	//bind shaders
	deviceContext->lpVtbl->VSSetShader(deviceContext, vertexShader, NULL, 0);
	deviceContext->lpVtbl->PSSetShader(deviceContext, pixelShader, NULL, 0);

}

void createVertexBufferAndAppendToList(vertex* vertexArray, int sizeInBytes)
{
	HRESULT hr;

	D3D11_BUFFER_DESC bd = {0};
	bd.ByteWidth = sizeInBytes;
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	bd.StructureByteStride = sizeof(vertex);

	D3D11_SUBRESOURCE_DATA sd = {0};
	sd.pSysMem = vertexArray;
	DXFUNCTIONFAILED(device->lpVtbl->CreateBuffer(device, &bd, &sd, &vertexBuffers[0]));
}

void createIndexDataBuffer(void* indexArray, int sizeInBytes)
{
	HRESULT hr;

	D3D11_BUFFER_DESC bd = { 0 };
	bd.ByteWidth = sizeInBytes;
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	bd.StructureByteStride = sizeof(int);

	D3D11_SUBRESOURCE_DATA sd = { 0 };
	sd.pSysMem = indexArray;
	DXFUNCTIONFAILED(device->lpVtbl->CreateBuffer(device, &bd, &sd, &IndexBuffer));
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

	if (renderTargetView)
	{
		renderTargetView->lpVtbl->Release(renderTargetView);
	}

	for (size_t i = 0; i < 32; i++)
	{
		if (vertexBuffers[i])
		{
			vertexBuffers[i]->lpVtbl->Release(vertexBuffers[i]);
		}
	}

	if (IndexBuffer)
	{
		IndexBuffer->lpVtbl->Release(IndexBuffer);
	}

	if (vertexShader)
	{
		vertexShader->lpVtbl->Release(vertexShader);
	}

	if (pixelShader)
	{
		pixelShader->lpVtbl->Release(pixelShader);
	}
}

void EndFrame()
{
	float colour[4] = {0.0f, 0.7f, 1.0f, 1.0f};
	deviceContext->lpVtbl->ClearRenderTargetView(deviceContext,renderTargetView, colour);

	//deviceContext->lpVtbl->DrawIndexed(deviceContext, 36, 0, 0);

	HRESULT hr;
	if (FAILED(hr = swapchain->lpVtbl->Present(swapchain, 1u, 0u)))
	{
		if (hr == DXGI_ERROR_DEVICE_REMOVED)
		{
			LOGDXMESSAGES();
			LOGWIN32EXCEPTION(RC_DX3D11_EXPCEPTION, device->lpVtbl->GetDeviceRemovedReason(device));
		}
		else
		{
			LOGDXMESSAGES();
			LOGWIN32EXCEPTION(RC_DX3D11_EXPCEPTION, hr);
		}
	}
}