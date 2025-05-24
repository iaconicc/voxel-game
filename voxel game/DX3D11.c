#include "DX3D11.h"
#include "Camera.h"
#include <d3dcompiler.h>

#define MODULE L"DX3D11"
#include "Logger.h"

#pragma comment(lib,"d3d11.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "D3DCompiler.lib")

HWND hwnd;
int windowedwidth;
int windowedheight;

IDXGISwapChain* swapchain = NULL;
ID3D11Device* device = NULL;
ID3D11DeviceContext* deviceContext = NULL;
ID3D11RenderTargetView* renderTargetView = NULL;

ID3D11DepthStencilState* depthStencilState;

//vertex and index buffers
ID3D11Buffer* vertexBuffers[32];
ID3D11Buffer* IndexBuffer = NULL;

//constant buffers (matrixes)
MatrixBuffers matrixBuffer;
ID3D11Buffer* DXMatrixBuffer = NULL;

ID3D11VertexShader* vertexShader = NULL;
ID3D11PixelShader* pixelShader = NULL;

ID3D11InputLayout* inputLayout = NULL;

UINT offset = 0;
UINT VertexSizeInBytes = sizeof(vertex);

static bool isWindowed = true;

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


static void CalculatePerspective(int width, int height)
{
	//calculate the perspective projection matrix
	float aspectRatio =(float) width /(float) height;
	float fov = glm_rad(70.0f);
	float nearPlane = 0.1f;
	float farPlane = 100.0f;

	glm_perspective(fov, aspectRatio, nearPlane, farPlane, matrixBuffer.projectionMatrix);
	glm_mat4_transpose(matrixBuffer.projectionMatrix);
}

static void CalculatePerspectiveAndSetViewport()
{
	//set viewport
	D3D11_VIEWPORT vp = { 0 };
	vp.Width = (float)sd.BufferDesc.Width;
	vp.Height = (float) sd.BufferDesc.Height;
	vp.MinDepth = 0;
	vp.MaxDepth = 1;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	deviceContext->lpVtbl->RSSetViewports(deviceContext, 1, &vp);

	CalculatePerspective(sd.BufferDesc.Width, sd.BufferDesc.Height);
}

static void CreateRenderTargetFromSwapChain()
{
	HRESULT hr;
	//getting swap chain buffer
	ID3D11Resource* backBuffer = NULL;
	DXFUNCTIONFAILED(swapchain->lpVtbl->GetBuffer(swapchain, 0, &IID_ID3D11Resource, &backBuffer));

	//create render target from back buffer
	DXFUNCTIONFAILED(device->lpVtbl->CreateRenderTargetView(device, backBuffer, NULL, &renderTargetView));
	backBuffer->lpVtbl->Release(backBuffer);
}

void toggleFullScreen()
{
	HRESULT hr;

	if (renderTargetView)
	{
		renderTargetView->lpVtbl->Release(renderTargetView);
		renderTargetView = NULL;
	}

	isWindowed = !isWindowed;

	DXGI_MODE_DESC md = { 0 };
	if (isWindowed) {
	DXFUNCTIONFAILED(swapchain->lpVtbl->SetFullscreenState(swapchain, FALSE, NULL));
	}
	if (!isWindowed) {
		DXFUNCTIONFAILED(swapchain->lpVtbl->SetFullscreenState(swapchain, TRUE, NULL));

		//making sure we get the highest resolution possible
		IDXGIOutput* output;
		DXFUNCTIONFAILED(swapchain->lpVtbl->GetContainingOutput(swapchain, &output));

		//checking how many modes are available
		UINT modesAvailable;
		DXFUNCTIONFAILED(output->lpVtbl->GetDisplayModeList(output, DXGI_FORMAT_B8G8R8A8_UNORM, 0, &modesAvailable, NULL));
		DXGI_MODE_DESC* modeList = malloc(sizeof(DXGI_MODE_DESC) * modesAvailable);
		DXFUNCTIONFAILED(output->lpVtbl->GetDisplayModeList(output, DXGI_FORMAT_B8G8R8A8_UNORM, 0, &modesAvailable, modeList));
		md = modeList[modesAvailable - 1];
		free(modeList);
		output->lpVtbl->Release(output);
	}
	else {
		md.Width = windowedwidth;
		md.Height = windowedheight;
	}

	// Resize the swap chain buffers
	DXFUNCTIONFAILED(swapchain->lpVtbl->ResizeBuffers(
		swapchain,
		0, // Use the same number of buffers
		md.Width, // Automatically match the window width
		md.Height, // Automatically match the window height
		DXGI_FORMAT_B8G8R8A8_UNORM,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
	));

	DXFUNCTIONFAILED(swapchain->lpVtbl->GetDesc(swapchain, &sd));

	if (isWindowed)
	{
		SetWindowLong(hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
		SetWindowPos(hwnd, HWND_TOP, 0, 0, sd.BufferDesc.Width, sd.BufferDesc.Height, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
	}

	CreateRenderTargetFromSwapChain();
	CalculatePerspectiveAndSetViewport();
}

void UpdateOnResize(int width, int height)
{
	windowedheight = height;
	windowedwidth = width;
	CalculatePerspective(width, height);
}


void CreateDX3D11DeviceForWindow(HWND hwnd, int width, int height)
{
	windowedheight = height;
	windowedwidth = width;

	hwnd = hwnd;
	sd.OutputWindow = hwnd;
	sd.Windowed = true;

	int DeviceFlags = 0;

#ifdef _DEBUG
	DeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	HRESULT result = D3D11CreateDeviceAndSwapChain(
		NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL, DeviceFlags,
		NULL, 0,
		D3D11_SDK_VERSION, &sd, &swapchain,
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

	//get updated swap chain descriptor
	DXFUNCTIONFAILED(swapchain->lpVtbl->GetDesc(swapchain, &sd));

	CalculatePerspectiveAndSetViewport();
	CreateRenderTargetFromSwapChain();

	//setup depth buffer
	D3D11_DEPTH_STENCIL_DESC dsDesc = { 0 };
	dsDesc.DepthEnable = true;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS;

	DXFUNCTIONFAILED(device->lpVtbl->CreateDepthStencilState(device, &dsDesc, &depthStencilState));
	
	deviceContext->lpVtbl->OMSetDepthStencilState(deviceContext, depthStencilState, 1u);

	//set render target view
	deviceContext->lpVtbl->OMSetRenderTargets(deviceContext, 1u, &renderTargetView, NULL);

	//set primitive topology
	deviceContext->lpVtbl->IASetPrimitiveTopology(deviceContext, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//create vertex shader
	ID3DBlob* vertexShaderBlob = NULL;
	DXFUNCTIONFAILED(D3DReadFileToBlob(L"VertexShader.cso", &vertexShaderBlob));
	DXFUNCTIONFAILED(device->lpVtbl->CreateVertexShader(device, vertexShaderBlob->lpVtbl->GetBufferPointer(vertexShaderBlob), vertexShaderBlob->lpVtbl->GetBufferSize(vertexShaderBlob), NULL, &vertexShader));

	//create and bind input layout
	D3D11_INPUT_ELEMENT_DESC ied[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};
	DXFUNCTIONFAILED(device->lpVtbl->CreateInputLayout(device, &ied, 1, vertexShaderBlob->lpVtbl->GetBufferPointer(vertexShaderBlob), vertexShaderBlob->lpVtbl->GetBufferSize(vertexShaderBlob), &inputLayout));
	deviceContext->lpVtbl->IASetInputLayout(deviceContext, inputLayout);

	//create buffer for matrixes
	D3D11_BUFFER_DESC mbd = { 0 };
	mbd.ByteWidth = sizeof(MatrixBuffers);
	mbd.Usage = D3D11_USAGE_DYNAMIC;
	mbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	mbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	mbd.MiscFlags = 0;
	mbd.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA msd = { 0 };
	msd.pSysMem = &matrixBuffer;
	DXFUNCTIONFAILED(device->lpVtbl->CreateBuffer(device, &mbd, &msd, &DXMatrixBuffer));

	deviceContext->lpVtbl->VSSetConstantBuffers(deviceContext, 0, 1, &DXMatrixBuffer);

	//create pixel shader
	ID3DBlob* pixelShaderBlob = NULL;
	DXFUNCTIONFAILED(D3DReadFileToBlob(L"PixelShader.cso", &pixelShaderBlob));
	DXFUNCTIONFAILED(device->lpVtbl->CreatePixelShader(device, pixelShaderBlob->lpVtbl->GetBufferPointer(pixelShaderBlob), pixelShaderBlob->lpVtbl->GetBufferSize(pixelShaderBlob), NULL, &pixelShader));

	//release raw shader binary
	vertexShaderBlob->lpVtbl->Release(vertexShaderBlob);
	pixelShaderBlob->lpVtbl->Release(pixelShaderBlob);

	//bind shaders
	deviceContext->lpVtbl->VSSetShader(deviceContext, vertexShader, NULL, 0);
	deviceContext->lpVtbl->PSSetShader(deviceContext, pixelShader, NULL, 0);

	LogInfo(L"pipeline set");
}


void createVertexBufferAndAppendToList(vertex* vertexArray, int sizeInBytes)
{
	HRESULT hr;

	D3D11_BUFFER_DESC bd = { 0 };
	bd.ByteWidth = sizeInBytes;
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	bd.StructureByteStride = sizeof(vertex);

	D3D11_SUBRESOURCE_DATA sd = { 0 };
	sd.pSysMem = vertexArray;
	DXFUNCTIONFAILED(device->lpVtbl->CreateBuffer(device, &bd, &sd, &vertexBuffers[0]));

	//bind vertex buffer
	deviceContext->lpVtbl->IASetVertexBuffers(deviceContext, 0, 1, &vertexBuffers, &VertexSizeInBytes, &offset);
}

int numberOfIndexes = 0;
void createIndexDataBuffer(void* indexArray, int sizeInBytes, int numberOfElements)
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

	//bind index buffer
	deviceContext->lpVtbl->IASetIndexBuffer(deviceContext, IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	numberOfIndexes = numberOfElements;
}


void DestroyDX3D11DeviceForWindow()
{
	if (swapchain)
	{
		swapchain->lpVtbl->Release(swapchain);
		swapchain = NULL;
	}

	if (device)
	{
		device->lpVtbl->Release(device);
		device = NULL;
	}

	if (deviceContext)
	{
		deviceContext->lpVtbl->Release(deviceContext);
		deviceContext = NULL;
	}

	if (renderTargetView)
	{
		renderTargetView->lpVtbl->Release(renderTargetView);
		renderTargetView = NULL;
	}

	for (size_t i = 0; i < 32; i++)
	{
		if (vertexBuffers[i])
		{
			vertexBuffers[i]->lpVtbl->Release(vertexBuffers[i]);
			vertexBuffers[i] = NULL;
		}
	}

	if (IndexBuffer)
	{
		IndexBuffer->lpVtbl->Release(IndexBuffer);
		IndexBuffer = NULL;
	}

	if (vertexShader)
	{
		vertexShader->lpVtbl->Release(vertexShader);
		vertexShader = NULL;
	}

	if (pixelShader)
	{
		pixelShader->lpVtbl->Release(pixelShader);
		pixelShader = NULL;
	}

	if (inputLayout)
	{
		inputLayout->lpVtbl->Release(inputLayout);
		inputLayout = NULL;
	}

	if (DXMatrixBuffer)
	{
		DXMatrixBuffer->lpVtbl->Release(DXMatrixBuffer);
		DXMatrixBuffer = NULL;
	}

	if (depthStencilState)
	{
		depthStencilState->lpVtbl->Release(depthStencilState);
		depthStencilState = NULL;
	}
}

float angle = 0;
static void updateMatrix()
{	
	D3D11_MAPPED_SUBRESOURCE map = {0};

	//calculate a transform matrixes
	glm_mat4_identity(matrixBuffer.transformationMatrix);

	vec3 translation = { 0.0f, 0.0f, 0.0f };
	glm_translate(matrixBuffer.transformationMatrix, translation);

	//angle += 0.1;
	//float angleRadians = glm_rad(0);
	//vec3 rotationAxis = { 0.0f, 1.0f, 0.0f };
	//glm_rotate(matrixBuffer.transformationMatrix, angleRadians, rotationAxis);

	//vec3 scale = { 1.0f, 1.0f, 1.0f };
	//glm_scale(matrixBuffer.transformationMatrix, scale);

	glm_mat4_transpose(matrixBuffer.transformationMatrix);

	//calculate view matrix
	vec3 cameraPosition; // Camera position in world space
	vec3 cameraTarget;   // Point the camera is looking at
	vec3 upVector = { 0.0f, 1.0f, 0.0f };       // Up direction
	getCameraTargetAndPosition(&cameraPosition, &cameraTarget);

	glm_lookat(cameraPosition, cameraTarget, upVector, matrixBuffer.viewMatrix);

	glm_mat4_transpose(matrixBuffer.viewMatrix);

	HRESULT hr;
	DXFUNCTIONFAILED(deviceContext->lpVtbl->Map(deviceContext, DXMatrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map));
	memcpy(map.pData, &matrixBuffer, sizeof(MatrixBuffers));
	deviceContext->lpVtbl->Unmap(deviceContext, DXMatrixBuffer, 0);
}

void EndFrame()
{
	updateMatrix();

	deviceContext->lpVtbl->OMSetRenderTargets(deviceContext, 1u, &renderTargetView, NULL);

	float colour[4] = {0.0f, 0.7f, 1.0f, 1.0f};
	deviceContext->lpVtbl->ClearRenderTargetView(deviceContext,renderTargetView, colour);

	deviceContext->lpVtbl->DrawIndexed(deviceContext, numberOfIndexes, 0, 0);

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