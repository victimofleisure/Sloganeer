// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      08nov25	initial version
        01      09nov25	add worker thread

*/

#pragma once

#include "D2DDevCtx.h"
#include "TextRenderer.h"

class CMeltProbe : public CTextRenderer {
public:
	CMeltProbe(ID2D1Factory1* pD2DFactory, IDWriteFactory* pDWriteFactory, ID2D1StrokeStyle1* pStrokeStyle);
	virtual ~CMeltProbe();
	bool	Create(CString sText, CString sFontName, float fFontSize, int nFontWeight, CD2DPointF ptDPI, float &fEraseStroke);
	IWICBitmap*	GetBitmap();
	CSize	GetBitmapSize() const;

protected:
// Overrides
	IFACEMETHOD_(ULONG, AddRef)() { return 1; }
	IFACEMETHOD_(ULONG, Release)() { return 1; }
    IFACEMETHOD(DrawGlyphRun)(void* pClientDrawingContext, FLOAT fBaselineOriginX, 
		FLOAT fBaselineOriginY, DWRITE_MEASURING_MODE measuringMode, DWRITE_GLYPH_RUN const* pGlyphRun, 
		DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, IUnknown* pClientDrawingEffect);

// Overrideables
	virtual void	OnError(HRESULT hr, LPCSTR pszSrcFileName, int nLineNum, LPCSTR pszSrcFileDate);

// Constants
	enum {
		AA_MARGIN = 1
	};

// Member data
	ID2D1Factory1*	m_pD2DFactory;	// Direct2D factory interface
	IDWriteFactory*	m_pDWriteFactory;	// DirectWrite factory interface
	ID2D1StrokeStyle1*	m_pStrokeStyle;	// stroke style interface
	CComPtr<ID2D1SolidColorBrush>	m_pBkgndBrush;	// background brush interface
	CComPtr<ID2D1SolidColorBrush>	m_pDrawBrush;	// drawing brush interface
	CComPtr<IDWriteTextFormat>	m_pTextFormat;	// text format interface
	CComPtr<IDWriteTextLayout>	m_pTextLayout;	// text layout interface
	CComPtr<ID2D1RenderTarget>	m_pRT;	// render target interface
	CComPtr<IWICImagingFactory> m_pWICFactory;	// WIC factory interface
	CComPtr<IWICBitmap>	m_pWICBmp;	// WIC bitmap interface
	CSize	m_szBmp;	// size of bitmap in pixels
	CD2DPointF	m_ptText;	// text origin in DIPs

// Helpers
	bool	ProbeText(float &fEraseStroke);
	bool	OutlineErasesText(float fOutlineStroke);
	static void	SimplifyText(CString& sText);
	bool	WriteBitmap(IWICBitmap* pBitmap, LPCTSTR pszImagePath);
};

inline IWICBitmap* CMeltProbe::GetBitmap()
{ 
	return m_pWICBmp;
}

inline CSize CMeltProbe::GetBitmapSize() const
{
	return m_szBmp;
}

class CMeltProbeWorker {
public:
	CMeltProbeWorker();
	~CMeltProbeWorker();
	bool	Create(const CStringArrayEx& aText, CString sFontName, float fFontSize, int nFontWeight, 
		CD2DPointF ptDPI, CArrayEx<float, float>& aStroke);
	void	Destroy();
	static HRESULT CreateStrokeStyle(ID2D1Factory1 *pD2DFactory, ID2D1StrokeStyle1 **ppStrokeStyle);

protected:
	class CProbeState {
	public:
		CProbeState();
		CStringArrayEx	m_aText;	// array of text strings to probe
		CString	m_sFontName;	// font name string
		float	m_fFontSize;	// font size
		int		m_nFontWeight;	// font weight
		CD2DPointF	m_ptDPI;	// caller's DPI
		CArrayEx<float, float>*	m_paStroke;	// pointer to result array
		bool	m_bIsCOMInit;	// true if COM initialization succeeded
		bool	Probe();
		void	OnError(HRESULT hr, LPCSTR pszSrcFileName, int nLineNum, LPCSTR pszSrcFileDate);
	};
	CAutoPtr<CWinThread>	m_pWorker;	// pointer to worker thread
	static UINT	ThreadFunc(LPVOID pParam);
};
