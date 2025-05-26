#include "TextureLoader.h"
#include <wincodec.h>

#define MODULE L"TexLoader"
#include "Logger.h"

void* LoadTextureFromFile(WCHAR* file, int* width, int* height)
{
	HRESULT hr;
	MSG* msg;

	//load texture
	IWICImagingFactory* imagingFactory = NULL;
	LOGWIN32FUNCTIONEXCEPTION(CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, &IID_IWICImagingFactory, &imagingFactory));

	IWICBitmapDecoder* bitmapDecoder = NULL;
	LOGWIN32FUNCTIONEXCEPTION(imagingFactory->lpVtbl->CreateDecoderFromFilename(imagingFactory, file, NULL, GENERIC_READ, 0, &bitmapDecoder));


	IWICBitmapFrameDecode* bitmapFrameDecode;
	LOGWIN32FUNCTIONEXCEPTION(bitmapDecoder->lpVtbl->GetFrame(bitmapDecoder, 0, &bitmapFrameDecode));


	//get format converter to convert
	IWICFormatConverter* formatConverter = NULL;
	LOGWIN32FUNCTIONEXCEPTION(imagingFactory->lpVtbl->CreateFormatConverter(imagingFactory, &formatConverter));

	LOGWIN32FUNCTIONEXCEPTION(formatConverter->lpVtbl->Initialize(formatConverter,
		(IWICBitmapSource*)bitmapFrameDecode,
		&GUID_WICPixelFormat32bppRGBA, // Convert to 32-bit RGBA
		WICBitmapDitherTypeNone,
		NULL,
		0.0f,
		WICBitmapPaletteTypeCustom
	));


	// Update the size and copy pixels from the format converter  
	LOGWIN32FUNCTIONEXCEPTION(formatConverter->lpVtbl->GetSize(formatConverter, (UINT*)width, (UINT*)height));


	int sizeInBytes = ((*width) * (*height)) * 4;
	void* temp = malloc(sizeInBytes);
	LOGWIN32FUNCTIONEXCEPTION(formatConverter->lpVtbl->CopyPixels(formatConverter, NULL, 4 * (*width), sizeInBytes, temp));

	// Release objects after use  
	formatConverter->lpVtbl->Release(formatConverter);
	bitmapFrameDecode->lpVtbl->Release(bitmapFrameDecode);
	bitmapDecoder->lpVtbl->Release(bitmapDecoder);
	imagingFactory->lpVtbl->Release(imagingFactory);
}