#include "BlockTexture.h"
#include <wincodec.h>

#define MODULE L"TextureLoader"
#include "Logger.h"

#define LOGWIN32FUNCTIONEXCEPTION(hrcall)
void LoadTextureAtlas(){
	HRESULT hr;
	MSG* msg;

	//load texture
	IWICImagingFactory* imagingFactory = NULL;
	if ((hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, &IID_IWICImagingFactory, &imagingFactory)) != S_OK)
	{
		LOGWIN32EXCEPTION(hr);
		return;
	}

	IWICBitmapDecoder* bitmapDecoder = NULL;
	if ((hr = imagingFactory->lpVtbl->CreateDecoderFromFilename(imagingFactory, L"ReferenceTexture.png", NULL, GENERIC_READ, 0, &bitmapDecoder)) != S_OK)
	{
		LOGWIN32EXCEPTION(hr);
		return;
	}

	IWICBitmapFrameDecode* bitmapFrameDecode;
	if ((hr = bitmapDecoder->lpVtbl->GetFrame(bitmapDecoder, 0, &bitmapFrameDecode)) != S_OK)
	{
		LOGWIN32EXCEPTION(hr);
		return;
	}

	IWICFormatConverter* formatConverter = NULL;
	hr = imagingFactory->lpVtbl->CreateFormatConverter(imagingFactory, &formatConverter);
	if (FAILED(hr)) {
		LOGWIN32EXCEPTION(hr);
		return;
	}

	hr = formatConverter->lpVtbl->Initialize(
		formatConverter,
		(IWICBitmapSource*)bitmapFrameDecode,
		&GUID_WICPixelFormat32bppRGBA, // Convert to 32-bit RGBA
		WICBitmapDitherTypeNone,
		NULL,
		0.0,
		WICBitmapPaletteTypeCustom
	);
	if (FAILED(hr)) {
		LOGWIN32EXCEPTION(hr);
		return;
	}

	int imageWidth = 0;
	int imageHeight = 0;
	// Update the size and copy pixels from the format converter
	hr = formatConverter->lpVtbl->GetSize(formatConverter, &imageWidth, &imageHeight);
	if (FAILED(hr)) {
		LOGWIN32EXCEPTION(hr);
		return;
	}

	int sizeInBytes = (imageWidth * imageHeight) * 4;
	void* temp = malloc(sizeInBytes);
	hr = formatConverter->lpVtbl->CopyPixels(formatConverter, NULL, 4 * imageWidth, sizeInBytes, temp);
	if (FAILED(hr)) {
		LOGWIN32EXCEPTION(hr);
		return;
	}

	// Release the format converter after use
	formatConverter->lpVtbl->Release(formatConverter);
}