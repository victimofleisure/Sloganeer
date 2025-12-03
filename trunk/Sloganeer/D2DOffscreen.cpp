// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00		13nov25	initial version
		01		03dec25	move auto unmap resource to helpers
		
*/

#include "stdafx.h"
#include "D2DOffscreen.h"
#include "D2DHelper.h"

#define CHECK(x) { HRESULT hr = x; if (FAILED(hr)) { OnError(hr, __FILE__, __LINE__, __DATE__); return false; }}

CD2DOffscreen::CD2DOffscreen()
{
	m_guidContainerFormat = GUID_ContainerFormatPng;
}

CD2DOffscreen::~CD2DOffscreen()
{
}

void CD2DOffscreen::OnError(HRESULT hr, LPCSTR pszSrcFileName, int nLineNum, LPCSTR pszSrcFileDate)
{
	UNREFERENCED_PARAMETER(hr);
	UNREFERENCED_PARAMETER(pszSrcFileName);
	UNREFERENCED_PARAMETER(nLineNum);
	UNREFERENCED_PARAMETER(pszSrcFileDate);
}

bool CD2DOffscreen::Create(D2D1_SIZE_U szImage, D2D1_SIZE_F szDPI)
{
	m_szImage = szImage;
	//
	// Create the WIC factory. WIC requires COM to be initialized, and the
	// calling thread is responsible for initializing COM before this call.
	//
	CHECK(m_pWICFactory.CoCreateInstance(CLSID_WICImagingFactory));
	//
	//	Create D3D device, D2D factory, and D2D device context.
	//
	CHECK(D3D11CreateDevice(
		NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
		NULL, 0, D3D11_SDK_VERSION, &m_pD3DDevice, NULL, NULL));
	CHECK(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory));
	CComPtr<IDXGIDevice1> pDXGIDevice;
	CHECK(m_pD3DDevice->QueryInterface(IID_PPV_ARGS(&pDXGIDevice)));
	CHECK(m_pD2DFactory->CreateDevice(pDXGIDevice, &m_pD2DDevice));
	CHECK(m_pD2DDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &m_pD2DDeviceContext));
	m_pD3DDevice->GetImmediateContext(&m_pD3DDeviceContext);	// cache device context
	//
	// Create the D3D render texture that D2D will draw into.
	//
	D3D11_TEXTURE2D_DESC td = {};
	td.Width = szImage.width;
	td.Height = szImage.height;
	td.MipLevels = 1;
	td.ArraySize = 1;
	td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	td.SampleDesc.Count = 1;	// no MSAA for WIC export
	td.SampleDesc.Quality = 0;
	td.Usage = D3D11_USAGE_DEFAULT;
	td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	td.CPUAccessFlags = 0;
	td.MiscFlags = 0;
    CHECK(m_pD3DDevice->CreateTexture2D(&td, NULL, &m_pRenderTex));
	// 
	// Create a staging texture for readback (same size/format).
	//
    td.BindFlags = 0;
    td.Usage = D3D11_USAGE_STAGING;
    td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    CHECK(m_pD3DDevice->CreateTexture2D(&td, NULL, &m_pStaging));
	//
	// Wrap the render texture as a DXGI surface, then as a D2D bitmap.
	//
    CComPtr<IDXGISurface> pSurface;
    CHECK(m_pRenderTex->QueryInterface(IID_PPV_ARGS(&pSurface)));
    D2D1_BITMAP_PROPERTIES1 bp = {};
    bp.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    bp.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
	bp.dpiX = szDPI.width;
	bp.dpiY = szDPI.height;
    bp.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET;
    bp.colorContext = NULL;
	CHECK(m_pD2DDeviceContext->CreateBitmapFromDxgiSurface(pSurface, &bp, &m_pTarget));
	//
	//	If we got here, everything succeeded; set the render target.
	//
	m_pD2DDeviceContext->SetTarget(m_pTarget);
	return true;
}

bool CD2DOffscreen::Write(LPCTSTR pszPath)
{
	m_pD3DDeviceContext->CopyResource(m_pStaging, m_pRenderTex);
	D3D11_MAPPED_SUBRESOURCE map = {};
	CHECK(m_pD3DDeviceContext->Map(m_pStaging, 0, D3D11_MAP_READ, 0, &map));
	CAutoUnmapResource	unmapStaging(m_pD3DDeviceContext, m_pStaging);	// dtor unmaps staging
	BYTE*	pData = static_cast<BYTE*>(map.pData);
	UINT	nPitch = map.RowPitch;
	UINT	nWidth = m_szImage.width;
	UINT	nHeight = m_szImage.height;
	//
	//	Create the WIC stream and encoder.
	//
	CComPtr<IWICStream>	pWICStream;
	CHECK(m_pWICFactory->CreateStream(&pWICStream));
	CHECK(pWICStream->InitializeFromFilename(pszPath, GENERIC_WRITE));	// requires UNICODE
	CComPtr<IWICBitmapEncoder> pEncoder;
    CHECK(m_pWICFactory->CreateEncoder(m_guidContainerFormat, NULL, &pEncoder));
    CHECK(pEncoder->Initialize(pWICStream, WICBitmapEncoderNoCache));
	//
	//	Create the WIC frame and set its attributes for our bitmap.
	//
	CComPtr<IWICBitmapFrameEncode> pFrame;
    CHECK(pEncoder->CreateNewFrame(&pFrame, NULL));
    CHECK(pFrame->Initialize(NULL));
    CHECK(pFrame->SetSize(nWidth, nHeight));
	WICPixelFormatGUID formatGUID = GUID_WICPixelFormat32bppBGRA;	// pixel format GUID
	CHECK(pFrame->SetPixelFormat(&formatGUID));	// set desired pixel format
	UINT cbBufferSize = nPitch * nHeight;	// buffer size is pitch times height
	//
	//	Write the pixels and commit.
	//
	CHECK(pFrame->WritePixels(nHeight, nPitch, cbBufferSize, pData));	// encode frame scanlines
    CHECK(pFrame->Commit());
    CHECK(pEncoder->Commit());
	return true;
}
