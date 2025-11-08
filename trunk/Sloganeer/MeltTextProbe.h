// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      08nov25	initial version

*/

#pragma once

#include "D2DDevCtx.h"
#include "TextRenderer.h"

class CMeltTextProbe : public CTextRenderer {
public:
	CMeltTextProbe(ID2D1Factory1* pD2DFactory, IDWriteFactory* pDWriteFactory, ID2D1StrokeStyle1* pStrokeStyle);
	virtual ~CMeltTextProbe();
	bool	Create(CString sText, CString sFontName, float fFontSize, int nFontWeight, CD2DPointF ptDPI, float &fEraseStroke);
	IWICBitmap*	GetBitmap();
	CSize	GetBitmapSize() const;
	bool	WriteBitmap(LPCTSTR pszImagePath);

protected:
// Overrides
	IFACEMETHOD_(ULONG, AddRef)() { return 1; }
	IFACEMETHOD_(ULONG, Release)() { return 1; }
    IFACEMETHOD(DrawGlyphRun)(void* pClientDrawingContext, FLOAT fBaselineOriginX, 
		FLOAT fBaselineOriginY, DWRITE_MEASURING_MODE measuringMode, DWRITE_GLYPH_RUN const* pGlyphRun, 
		DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, IUnknown* pClientDrawingEffect);
	virtual void	OnError(HRESULT hr, LPCSTR pszSrcFileName, int nLineNum, LPCSTR pszSrcFileDate);

// Constants
	enum {
		AA_MARGIN = 1
	};

// Member data
	ID2D1Factory1*	m_pD2DFactory;
	IDWriteFactory*	m_pDWriteFactory;
	ID2D1StrokeStyle1*	m_pStrokeStyle;
	CComPtr<ID2D1SolidColorBrush>	m_pBkgndBrush;
	CComPtr<ID2D1SolidColorBrush>	m_pDrawBrush;
	CComPtr<IDWriteTextFormat>	m_pTextFormat;
	CComPtr<IDWriteTextLayout>	m_pTextLayout;
	CComPtr<ID2D1RenderTarget>	m_pRT;
	CComPtr<IWICImagingFactory> m_pWICFactory;
	CComPtr<IWICBitmap>	m_pWICBmp;
	CSize	m_szBmp;
	CD2DPointF	m_ptText;

// Helpers
	bool	OutlineErasesText(float fOutlineStroke);
};

inline IWICBitmap* CMeltTextProbe::GetBitmap()
{ 
	return m_pWICBmp;
}

inline CSize CMeltTextProbe::GetBitmapSize() const
{
	return m_szBmp;
}
