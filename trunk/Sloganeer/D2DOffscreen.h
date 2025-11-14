// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00		13nov25	initial version
		
*/

#pragma once

#include "d2d1_1.h"
#include "d3d11.h"
#include "dxgi1_2.h"

class CD2DOffscreen : public WObject {
public:
// Construction
	CD2DOffscreen();
	virtual	~CD2DOffscreen();
	bool	Create(D2D1_SIZE_U szImage, D2D1_SIZE_F szDPI = CD2DSizeF(96, 96));

// Attributes
	bool	IsCreated() const;
	void	SetFormat(GUID guidContainerFormat);

// Operations
	bool	Write(LPCTSTR pszPath);

// Public data
	CComPtr<ID2D1Factory1>	m_pD2DFactory;	// Direct2D factory
    CComPtr<ID2D1DeviceContext>	m_pD2DDeviceContext;	// Direct2D device context

protected:
// Types
	// helper class to automatically unmap a resource when the helper's instance
	// is destroyed; the resource must be mapped, otherwise behavior is undefined
	class CAutoUnmapResource {
	public:
		CAutoUnmapResource(ID3D11DeviceContext* pD3DDC, ID3D11Resource* pD3DRes);
		~CAutoUnmapResource();

	protected:
		ID3D11DeviceContext*	m_pD3DDC;	// pointer to Direct3D device context
		ID3D11Resource*	m_pD3DRes;	// pointer to Direct3D resource to be unmapped
	};

// Member data
	CComPtr<ID3D11Device> m_pD3DDevice;	// Direct3D device
	CComPtr<ID2D1Device>	m_pD2DDevice;	// Direct2D device
    CComPtr<ID3D11Texture2D>	m_pRenderTex;	 // GPU render target
	CComPtr<ID3D11Texture2D>	m_pStaging; // GPU to CPU staging texture
	CComPtr<ID2D1Bitmap1>	m_pTarget;  // D2D target bitmap
	CComPtr<IWICImagingFactory>	m_pWICFactory;	// WIC imaging factory
	D2D1_SIZE_U	m_szImage;		// size of image in pixels
	GUID	m_guidContainerFormat;	// identifier of WIC container format

// Overrideables
	virtual void	OnError(HRESULT hr, LPCSTR pszSrcFileName, int nLineNum, LPCSTR pszSrcFileDate);
};

inline bool CD2DOffscreen::IsCreated() const
{
	return m_pTarget != NULL;
}

inline void CD2DOffscreen::SetFormat(GUID guidContainerFormat)
{
	m_guidContainerFormat = guidContainerFormat;
}
