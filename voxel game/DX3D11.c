#include "DX3D11.h"
#include "Camera.h"
#include "BlockTexture.h"
#include <d3dcompiler.h>
#include <d3d11_4.h>
#include <sys/timeb.h>
#include "world.h"

#define MODULE L"DX3D11"
#include "Logger.h"

#pragma comment(lib,"d3d11.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "D3DCompiler.lib")
#pragma comment(lib, "Windowscodecs.lib")

bool settingUp = true;

HWND hwnd;
int windowedwidth;
int windowedheight;

float colour[4] = { 0.0f, 0.7f, 1.0f, 1.0f };

typedef struct {
	mat4 transformationMatrix;
	mat4 projectionMatrix;
	mat4 viewMatrix;
}MatrixBuffers;

typedef struct {
	vec4 Fogcolour;
	vec3 CamPos;
	float FogDensity;
	float FogEnd;
}FogConstants;

ID3D11Multithread* thread = NULL;

IDXGISwapChain* swapchain = NULL;
ID3D11Device* device = NULL;
ID3D11DeviceContext* deviceContext = NULL;
ID3D11RenderTargetView* renderTargetView = NULL;

ID3D11DepthStencilState* depthStencilState;
ID3D11Texture2D* depthTexture;
ID3D11DepthStencilView* depthStencilView;

//constant buffers (matrixes)
MatrixBuffers matrixBuffer;
ID3D11Buffer* DXMatrixBuffer = NULL;

//fog constants
FogConstants fog = {
	{0.0f, 0.0f, 0.0f, 0.0f},
	{0.0f, 0.0f, 0.0f},
	0.45f,
	200.0f,
};
ID3D11Buffer* FogConstantBuffer = NULL;

ID3D11Texture2D* texture = NULL;
ID3D11ShaderResourceView* shaderResourceView = NULL;

ID3D11VertexShader* vertexShader = NULL;
ID3D11PixelShader* pixelShader = NULL;
ID3D11SamplerState* samplerState = NULL;

ID3D11InputLayout* inputLayout = NULL;

IDXGIFactory* dxgiFactory = NULL;

UINT offset = 0;
UINT VertexSizeInBytes = sizeof(vertex);

bool isWindowed = true;
bool render = true;

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

#define DXFUNCTIONFAILED(hrcall) if(FAILED(hr = (hrcall))){ LOGDXMESSAGES(); LOGWIN32EXCEPTION(hr); return;}
static void CalculatePerspective(int width, int height)
{
	//calculate the perspective projection matrix
	float aspectRatio =(float) width /(float) height;
	float fov = glm_rad(70.0f);
	float nearPlane = 0.1f;
	float farPlane = 1000.0f;

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

static void createDepthTexture()
{
	HRESULT hr;
	MSG* msg;

	//create depth texture
	D3D11_TEXTURE2D_DESC dtd = { 0 };
	dtd.Height = sd.BufferDesc.Height;
	dtd.Width = sd.BufferDesc.Width;
	dtd.MipLevels = 1;
	dtd.ArraySize = 1;
	dtd.Format = DXGI_FORMAT_D32_FLOAT;
	dtd.SampleDesc.Count = 1;
	dtd.SampleDesc.Quality = 0;
	dtd.Usage = D3D11_USAGE_DEFAULT;
	dtd.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	DXFUNCTIONFAILED(device->lpVtbl->CreateTexture2D(device, &dtd, NULL, &depthTexture));

	//create depth texture view
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvd = { 0 };
	dsvd.Format = DXGI_FORMAT_D32_FLOAT;
	dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvd.Texture2D.MipSlice = 0;
	DXFUNCTIONFAILED(device->lpVtbl->CreateDepthStencilView(device, depthTexture, &dsvd, &depthStencilView));
}

static void SetDepthStateAndCreateDepthTexture()
{
	HRESULT hr;
	MSG* msg;

	//setup depth buffer
	D3D11_DEPTH_STENCIL_DESC dsDesc = { 0 };
	dsDesc.DepthEnable = true;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS;

	DXFUNCTIONFAILED(device->lpVtbl->CreateDepthStencilState(device, &dsDesc, &depthStencilState));

	deviceContext->lpVtbl->OMSetDepthStencilState(deviceContext, depthStencilState, 1u);

	createDepthTexture();
}

static void CreateRenderTargetFromSwapChain()
{
	HRESULT hr;
	WCHAR* msg;

	//getting swap chain buffer
	ID3D11Resource* backBuffer = NULL;
	DXFUNCTIONFAILED(swapchain->lpVtbl->GetBuffer(swapchain, 0, &IID_ID3D11Resource, &backBuffer));

	//create render target from back buffer
	DXFUNCTIONFAILED(device->lpVtbl->CreateRenderTargetView(device, backBuffer, NULL, &renderTargetView));
	backBuffer->lpVtbl->Release(backBuffer);
}


static void resizeSwapchain(int width, int height)
{
	HRESULT hr;
	WCHAR* msg;

	if (swapchain)
	{
		DXGI_MODE_DESC md = { 0 };
		if (!isWindowed)
		{
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
		}else{
			md.Width = width;
			md.Height = height;
		}

		if (depthTexture) {
			depthTexture->lpVtbl->Release(depthTexture);
			depthTexture = NULL;
		}

		if (depthStencilView) {
			depthStencilView->lpVtbl->Release(depthStencilView);
			depthStencilView = NULL;
		}

		if (renderTargetView) {
			renderTargetView->lpVtbl->Release(renderTargetView);
			renderTargetView = NULL;
		}

		LogDebug(L"(%u , %u)", md.Width, md.Height);
		DXFUNCTIONFAILED(swapchain->lpVtbl->ResizeBuffers(
			swapchain,
			0, // Use the same number of buffers
			md.Width,
			md.Height, 
			DXGI_FORMAT_B8G8R8A8_UNORM,
			DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
		));

		DXFUNCTIONFAILED(swapchain->lpVtbl->GetDesc(swapchain, &sd));

		CreateRenderTargetFromSwapChain();
		CalculatePerspectiveAndSetViewport();
		createDepthTexture();
	}
}

bool wasTabbing = false;
bool switching = false;
void setInactive(){
	if (!isWindowed){
		toggleFullScreen();
		wasTabbing = true;
	}
	render = false;
}

void setactive() {
	if (!switching)
	{
		if (wasTabbing) {
			wasTabbing = false;
			toggleFullScreen();
		}
	}
	render = true;
}

void toggleFullScreen()
{
	HRESULT hr;
	WCHAR* msg;
	
	switching = true;
	isWindowed = !isWindowed;

	if (isWindowed) {
		if (swapchain)
		{
			DXFUNCTIONFAILED(swapchain->lpVtbl->SetFullscreenState(swapchain, FALSE, NULL));
		}
		else{
			return;
		}
	}

	DXGI_MODE_DESC md = { 0 };
	//making sure we get the highest resolution possible when in fullscreen mode
	if (!isWindowed) {
		DXFUNCTIONFAILED(swapchain->lpVtbl->SetFullscreenState(swapchain, TRUE, NULL));
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

	resizeSwapchain(md.Width, md.Height);

	if (isWindowed)
	{
		SetWindowLong(hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
		SetWindowPos(hwnd, HWND_TOP, 0, 0, sd.BufferDesc.Width, sd.BufferDesc.Height, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
	}

}

void UpdateOnResize(int width, int height)
{
	windowedheight = height;
	windowedwidth = width;
	CalculatePerspective(width, height);
}

struct _timeb lastEpoch, newEpoch;
void CreateDX3D11DeviceForWindow(HWND hwnd, int width, int height)
{
	windowedheight = height;
	windowedwidth = width;

	hwnd = hwnd;
	sd.OutputWindow = hwnd;
	sd.Windowed = isWindowed;

	

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

	//for debugging macro
	WCHAR* msg;
	HRESULT hr;

	if (FAILED(result))
	{
		LOGDXMESSAGES();
		LOGWIN32EXCEPTION(result);
		return;
	}

	IDXGIDevice* dxgiDevice = NULL;
	DXFUNCTIONFAILED(device->lpVtbl->QueryInterface(device,&IID_IDXGIDevice, (void**)&dxgiDevice));

	IDXGIAdapter* dxgiAdapter = NULL;
	DXFUNCTIONFAILED(dxgiDevice->lpVtbl->GetParent(dxgiDevice,&IID_IDXGIAdapter, (void**)&dxgiAdapter));
	
	DXFUNCTIONFAILED(dxgiAdapter->lpVtbl->GetParent(dxgiAdapter,&IID_IDXGIFactory, (void**)&dxgiFactory));

	dxgiDevice->lpVtbl->Release(dxgiDevice);
	dxgiAdapter->lpVtbl->Release(dxgiAdapter);

	LogInfo(L"DX3D device created succesfully");

	LogInfo(L"setting up base pipeline...");
	//setup pipeline


	//load texture atlas
	int imageWidth = 0;
	int imageHeight = 0;
	void* textureAtlasBitmap = NULL;
	if (!(textureAtlasBitmap = LoadTextureAtlas(&imageWidth, &imageHeight))){
		return;
	}

	//DXFUNCTIONFAILED(deviceContext->lpVtbl->QueryInterface(deviceContext, &IID_ID3D11Multithread, &thread));
	//thread->lpVtbl->SetMultithreadProtected(thread, true);

	//create texture resource
	D3D11_TEXTURE2D_DESC td = {0};
	td.Width = imageWidth;
	td.Height = imageHeight;
	td.MipLevels = 0;
	td.ArraySize = 1;
	td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	td.SampleDesc.Count = 1;
	td.SampleDesc.Quality = 0;
	td.Usage = D3D11_USAGE_DEFAULT;
	td.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	td.CPUAccessFlags = 0;
	td.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

	D3D11_SUBRESOURCE_DATA tsd = {0};
	tsd.pSysMem = textureAtlasBitmap;
	tsd.SysMemPitch = 4 * imageWidth;

	DXFUNCTIONFAILED(device->lpVtbl->CreateTexture2D(device, &td, NULL, &texture));

	deviceContext->lpVtbl->UpdateSubresource(deviceContext, texture, 0u, NULL, textureAtlasBitmap, 4*imageWidth, 0u);

	//create a shader resource for the texture
	D3D11_SHADER_RESOURCE_VIEW_DESC rvd = {0};
	rvd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	rvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	rvd.Texture2D.MostDetailedMip = 0;
	rvd.Texture2D.MipLevels = -1;
	DXFUNCTIONFAILED(device->lpVtbl->CreateShaderResourceView(device, texture, &rvd, &shaderResourceView));

	deviceContext->lpVtbl->GenerateMips(deviceContext, shaderResourceView);

	D3D11_SAMPLER_DESC  samplerDesc = {0};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MaxAnisotropy = 0;
	samplerDesc.MipLODBias = -0.10f;
	samplerDesc.MinLOD = 0.0f;
	samplerDesc.MaxLOD = 3.0f;

	DXFUNCTIONFAILED(device->lpVtbl->CreateSamplerState(device, &samplerDesc, &samplerState));

	//free atlas bitmap
	free(textureAtlasBitmap);

	deviceContext->lpVtbl->PSSetSamplers(deviceContext, 0, 1, &samplerState);
	deviceContext->lpVtbl->PSSetShaderResources(deviceContext, 0, 1, &shaderResourceView);

	//set primitive topology
	deviceContext->lpVtbl->IASetPrimitiveTopology(deviceContext, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//get updated swap chain descriptor
	DXFUNCTIONFAILED(swapchain->lpVtbl->GetDesc(swapchain, &sd));

	CreateRenderTargetFromSwapChain();
	SetDepthStateAndCreateDepthTexture();
	CalculatePerspectiveAndSetViewport();



	//create vertex shader
	ID3DBlob* vertexShaderBlob = NULL;
	DXFUNCTIONFAILED(D3DReadFileToBlob(L"VertexShader.cso", &vertexShaderBlob));
	DXFUNCTIONFAILED(device->lpVtbl->CreateVertexShader(device, vertexShaderBlob->lpVtbl->GetBufferPointer(vertexShaderBlob), vertexShaderBlob->lpVtbl->GetBufferSize(vertexShaderBlob), NULL, &vertexShader));

	//create and bind input layout
	const D3D11_INPUT_ELEMENT_DESC ied[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TexCoord", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};
	DXFUNCTIONFAILED(device->lpVtbl->CreateInputLayout(device, &ied, 2, vertexShaderBlob->lpVtbl->GetBufferPointer(vertexShaderBlob), vertexShaderBlob->lpVtbl->GetBufferSize(vertexShaderBlob), &inputLayout));

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

	//create buffer for fog constants
	D3D11_BUFFER_DESC fbd = { 0 };
	fbd.ByteWidth = sizeof(FogConstants);
	fbd.Usage = D3D11_USAGE_DYNAMIC;
	fbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	fbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	fbd.MiscFlags = 0;
	fbd.StructureByteStride = 0;

	fog.Fogcolour[0] = colour[0];
	fog.Fogcolour[1] = colour[1];
	fog.Fogcolour[2] = colour[2];
	fog.Fogcolour[3] = colour[3];

	D3D11_SUBRESOURCE_DATA fsd = { 0 };
	fsd.pSysMem = &fog;
	DXFUNCTIONFAILED(device->lpVtbl->CreateBuffer(device, &fbd, &fsd, &FogConstantBuffer));

	deviceContext->lpVtbl->VSSetConstantBuffers(deviceContext, 0, 1, &DXMatrixBuffer);
	deviceContext->lpVtbl->PSSetConstantBuffers(deviceContext, 0, 1, &FogConstantBuffer);

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

#ifdef _DEBUG
	getDxgiDebug()->lpVtbl->ReportLiveObjects(getDxgiDebug(), DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
#endif

	_ftime64_s(&lastEpoch);
	settingUp = false;
}

bool DxsettingUp()
{
	return settingUp;
}


ID3D11Buffer* createVertexBuffer(vertex* vertexArray, int sizeInBytes)
{
	HRESULT hr;
	WCHAR* msg;

	ID3D11Buffer* vertexBuffer;

	D3D11_BUFFER_DESC bd = { 0 };
	bd.ByteWidth = sizeInBytes;
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	bd.StructureByteStride = sizeof(vertex);

	D3D11_SUBRESOURCE_DATA sd = { 0 };
	sd.pSysMem = vertexArray;
	DXFUNCTIONFAILED(device->lpVtbl->CreateBuffer(device, &bd, &sd, &vertexBuffer));

	return vertexBuffer;
}


typedef struct {
	ID3D11Buffer** vertexBuffer;
	ID3D11Buffer** indexBuffer;
}AllocatedBuffers;

AllocatedBuffers AllocateBuffers(int BufferCount, int vertexMin, int indexMin){
	ID3D11Buffer** AllocatedVertexBuffers = malloc(sizeof(ID3D11Buffer*) * BufferCount);
	ID3D11Buffer** AllocatedIndexBuffers = malloc(sizeof(ID3D11Buffer*) * BufferCount);
	if(!AllocatedVertexBuffers || !AllocatedIndexBuffers){
		if(AllocatedVertexBuffers){
			free(AllocatedVertexBuffers);
		}else if(AllocatedIndexBuffers){
			free(AllocatedIndexBuffers);
		}
		
		AllocatedBuffers buffers = {0};
		return buffers;
	}

	D3D11_BUFFER_DESC ibd = { 0 };
	ibd.ByteWidth = indexMin;
	ibd.Usage = D3D11_USAGE_DEFAULT;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	ibd.StructureByteStride = sizeof(int);

	D3D11_BUFFER_DESC vbd = { 0 };
	vbd.ByteWidth = vertexMin;
	vbd.Usage = D3D11_USAGE_DEFAULT;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = sizeof(vertex);

	HRESULT hr;
	WCHAR* msg;
	for (int i = 0; i < BufferCount; i++){
		DXFUNCTIONFAILED(device->lpVtbl->CreateBuffer(device, &vbd, NULL, &AllocatedVertexBuffers[i]));
		DXFUNCTIONFAILED(device->lpVtbl->CreateBuffer(device, &ibd, NULL, &AllocatedIndexBuffers[i]));
	}

	AllocatedBuffers buffers = {
	.indexBuffer = AllocatedIndexBuffers,
	.vertexBuffer = AllocatedVertexBuffers,
	};

	return buffers;
}

ID3D11Buffer* createIndexDataBuffer(int* indexArray, int sizeInBytes)
{
	HRESULT hr;
	WCHAR* msg;

	ID3D11Buffer* indexBuffer;

	D3D11_BUFFER_DESC bd = { 0 };
	bd.ByteWidth = sizeInBytes;
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	bd.StructureByteStride = sizeof(int);

	D3D11_SUBRESOURCE_DATA sd = { 0 };
	sd.pSysMem = indexArray;
	DXFUNCTIONFAILED(device->lpVtbl->CreateBuffer(device, &bd, &sd, &indexBuffer));

	return indexBuffer;
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
	if (texture)
	{
		texture->lpVtbl->Release(texture);
		texture = NULL;
	}
	if (shaderResourceView)
	{
		shaderResourceView->lpVtbl->Release(shaderResourceView);
		shaderResourceView = NULL;
	}
	if (samplerState)
	{
		samplerState->lpVtbl->Release(samplerState);
		samplerState = NULL;
	}
	if (depthStencilView)
	{
		depthStencilView->lpVtbl->Release(depthStencilView);
		depthStencilView = NULL;
	}
	if (depthTexture)
	{
		depthTexture->lpVtbl->Release(depthTexture);
		depthTexture = NULL;
	}
	if (dxgiFactory)
	{
		dxgiFactory->lpVtbl->Release(dxgiFactory);
		dxgiFactory = NULL;
	}
	if (thread)
	{
		thread->lpVtbl->Release(thread);
		thread = NULL;
	}
	if (FogConstantBuffer)
	{
		FogConstantBuffer->lpVtbl->Release(FogConstantBuffer);
		FogConstantBuffer = NULL;
	}
}

static void updateTransformMatrix(vec3 pos)
{	
	//calculate a transform matrixes
	glm_mat4_identity(matrixBuffer.transformationMatrix);
	glm_translate(matrixBuffer.transformationMatrix, pos);
	glm_mat4_transpose(matrixBuffer.transformationMatrix);

	HRESULT hr;
	WCHAR* msg;
	D3D11_MAPPED_SUBRESOURCE map = { 0 };

	DXFUNCTIONFAILED(deviceContext->lpVtbl->Map(deviceContext, DXMatrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map));
	memcpy(map.pData, &matrixBuffer, sizeof(MatrixBuffers));
	deviceContext->lpVtbl->Unmap(deviceContext, DXMatrixBuffer, 0);
}

static void updateViewMatrix()
{
	HRESULT hr;
	WCHAR* msg;

	//calculate view matrix
	vec3 cameraPosition; // Camera position in world space
	vec3 cameraTarget;   // Point the camera is looking at
	vec3 upVector = { 0.0f, 1.0f, 0.0f };       // Up direction
	getCameraTargetAndPosition(&cameraPosition, &cameraTarget);

	glm_vec3_copy(cameraPosition, fog.CamPos);
	glm_lookat(cameraPosition, cameraTarget, upVector, matrixBuffer.viewMatrix);
	glm_mat4_transpose(matrixBuffer.viewMatrix);

	D3D11_MAPPED_SUBRESOURCE MatrixMap = { 0 };
	D3D11_MAPPED_SUBRESOURCE FogMap = { 0 };
	DXFUNCTIONFAILED(deviceContext->lpVtbl->Map(deviceContext, DXMatrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MatrixMap));
	DXFUNCTIONFAILED(deviceContext->lpVtbl->Map(deviceContext, FogConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &FogMap));
	memcpy(MatrixMap.pData, &matrixBuffer, sizeof(MatrixBuffers));
	memcpy(FogMap.pData, &fog, sizeof(FogConstants));
	deviceContext->lpVtbl->Unmap(deviceContext, DXMatrixBuffer, 0);
	deviceContext->lpVtbl->Unmap(deviceContext, FogConstantBuffer, 0);
}

void DrawMesh(ID3D11Buffer* vertexBuffer, ID3D11Buffer* indexBuffer, int indexBufferElements, vec3 pos)
{
		deviceContext->lpVtbl->IASetVertexBuffers(deviceContext, 0, 1, &vertexBuffer, &VertexSizeInBytes, &offset);
		deviceContext->lpVtbl->IASetIndexBuffer(deviceContext, indexBuffer, DXGI_FORMAT_R32_UINT, 0);

		updateTransformMatrix(pos);

		deviceContext->lpVtbl->OMSetRenderTargets(deviceContext, 1u, &renderTargetView, depthStencilView);
		deviceContext->lpVtbl->DrawIndexed(deviceContext, indexBufferElements, 0, 0);
}

float deltaTime = 0;
float frameRate = 0;
void EndFrame()
{
	HRESULT hr;
	WCHAR* msg;

	updateViewMatrix();

	if (render)
	{
		deviceContext->lpVtbl->OMSetRenderTargets(deviceContext, 1u, &renderTargetView, depthStencilView);
		if (FAILED(hr = swapchain->lpVtbl->Present(swapchain, 1u, 0)))
		{
			if (hr == DXGI_ERROR_DEVICE_REMOVED)
			{
				LOGDXMESSAGES();
				LOGWIN32EXCEPTION(device->lpVtbl->GetDeviceRemovedReason(device));
			}
			else
			{
				LOGDXMESSAGES();
				LOGWIN32EXCEPTION(hr);
			}
		}
		deviceContext->lpVtbl->ClearDepthStencilView(deviceContext, depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
		deviceContext->lpVtbl->ClearRenderTargetView(deviceContext, renderTargetView, colour);
		
	}

	switching = false;
	
	_ftime64_s(&newEpoch);
	deltaTime = ((float)(newEpoch.time - lastEpoch.time)) + (((float)(newEpoch.millitm - lastEpoch.millitm)) / 1000);
	lastEpoch = newEpoch;
	frameRate = deltaTime > 0 ? (1.0f / deltaTime) : 0.0f;
}

float getFrameDelta(){
	return deltaTime;
}

float getFrameRate(){
	return frameRate;
}