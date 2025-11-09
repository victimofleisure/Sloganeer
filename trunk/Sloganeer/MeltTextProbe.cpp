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

#include "stdafx.h"
#include "Sloganeer.h"
#include "MeltTextProbe.h"
#include "Benchmark.h"
#include <bitset>

#define DTF(x) static_cast<float>(x)

#define CHECK(x) { HRESULT hr = x; if (FAILED(hr)) { OnError(hr, __FILE__, __LINE__, __DATE__); return false; }}

#define WRITE_PROBE_BITMAP	0	// non-zero to write probe bitmap to a file for debugging

CMeltTextProbe::CMeltTextProbe(ID2D1Factory1* pD2DFactory, IDWriteFactory* pDWriteFactory, ID2D1StrokeStyle1* pStrokeStyle)
{ 
	m_pD2DFactory = pD2DFactory;
	m_pDWriteFactory = pDWriteFactory;
	m_pStrokeStyle = pStrokeStyle;
	m_szBmp = CSize(0, 0);
	m_ptText = CD2DPointF(0, 0);
}

CMeltTextProbe::~CMeltTextProbe()
{
}

void CMeltTextProbe::OnError(HRESULT hr, LPCSTR pszSrcFileName, int nLineNum, LPCSTR pszSrcFileDate)
{
	UNREFERENCED_PARAMETER(hr);
	UNREFERENCED_PARAMETER(pszSrcFileName);
	UNREFERENCED_PARAMETER(nLineNum);
	UNREFERENCED_PARAMETER(pszSrcFileDate);
}

bool CMeltTextProbe::Create(CString sText, CString sFontName, float fFontSize, int nFontWeight, CD2DPointF ptDPI, float &fEraseStroke)
{
	SimplifyText(sText);	// remove whitespace and duplicate characters
	if (sText.IsEmpty())	// if empty text
		return false;	// probe is meaningless
	// create text format instance
	CHECK(m_pDWriteFactory->CreateTextFormat(
		sFontName,	// font name
		NULL,	// font collection
		static_cast<DWRITE_FONT_WEIGHT>(nFontWeight),	// font weight
		DWRITE_FONT_STYLE_NORMAL,	// font style
		DWRITE_FONT_STRETCH_NORMAL,	// font stretch
		fFontSize,	// font size in points
		L"",	// locale
		&m_pTextFormat	// receives text format instance
	));
	// create text layout instance
	CHECK(m_pDWriteFactory->CreateTextLayout(
		sText,			// source text
		sText.GetLength(),	// text length in characters
		m_pTextFormat,	// text format instance
		10000.0f,		// layout box width in DIPs
		10000.0f,		// layout box height in DIPs
		&m_pTextLayout	// receives text layout instance
	));
	DWRITE_TEXT_METRICS	textMetrics;	// text metrics
	CHECK(m_pTextLayout->GetMetrics(&textMetrics));	// get text metrics
	DWRITE_OVERHANG_METRICS	overhangMetrics;	// overhang metrics
	CHECK(m_pTextLayout->GetOverhangMetrics(&overhangMetrics));	// get overhang metrics
	CHECK(m_pWICFactory.CoCreateInstance(CLSID_WICImagingFactory));	// create WIC factory
	// calculate bitmap size
	CD2DSizeF	szBmp(
		textMetrics.layoutWidth + overhangMetrics.left + overhangMetrics.right + AA_MARGIN * 2, 
		textMetrics.layoutHeight + overhangMetrics.top + overhangMetrics.bottom + AA_MARGIN * 2);
	m_szBmp = CSize(Round(ceil(szBmp.width)), Round(ceil(szBmp.height)));
	// create bitmap
	CHECK(m_pWICFactory->CreateBitmap(m_szBmp.cx, m_szBmp.cy,
		GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnLoad, &m_pWICBmp));
	// create render target for bitmap
	D2D1_RENDER_TARGET_PROPERTIES props =
		D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_SOFTWARE,
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), ptDPI.x, ptDPI.y);
	m_pD2DFactory->CreateWicBitmapRenderTarget(m_pWICBmp, props, &m_pRT);
	CD2DSizeF	szRT(m_pRT->GetSize());	// get render target size
    m_pRT->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1), &m_pDrawBrush);
    m_pRT->CreateSolidColorBrush(D2D1::ColorF(0), &m_pBkgndBrush);
	m_ptText = CD2DPointF(overhangMetrics.left + AA_MARGIN, overhangMetrics.top + AA_MARGIN);
#if WRITE_PROBE_BITMAP
	OutlineErasesText(0.0f);
	WriteBitmap(m_pWICBmp, _T("meltprobe.tif"));
#endif
	return ProbeText(fEraseStroke);
}

bool CMeltTextProbe::ProbeText(float &fEraseStroke)
{
	// To establish an upper limit for the outline stroke, we increase it
	// by powers of two until it's big enough to completely erase the text.
	//
	float	fStroke;
	const int	nTries = 10;	// maximum stroke of 2 ^ nTries
	int	iTry;
	for (iTry = 0; iTry < nTries; iTry++) {
		fStroke = static_cast<float>(1 << iTry);
		if (OutlineErasesText(fStroke))	// try to erase text
			break;	// we're done
	}
	fEraseStroke = fStroke;	// update result with upper limit stroke
	if (iTry >= nTries)	// if text wasn't erased after all tries
		return false;	// maximum stroke isn't big enough
	if (!iTry)	// if text was erased on first try
		return true;	// text must be tiny, but we're done
	//
	// We've established the outline stroke's upper limit. Stroke 2 ^ iTry
	// completely erased the text, whereas stroke 2 ^ (iTry - 1) did not.
	// Now refine the outline stroke using binary search; repeat until the
	// difference between the high and low strokes is reduced to a tolerance.
	//
	float	fHighStroke = fStroke;	// stroke that erases text
	float	fLowStroke = static_cast<float>(1 << (iTry - 1));	// stroke that doesn't erase text
	const float	fStrokeTolerance = 0.5f;	// search ends when this tolerance is achieved
	while (fHighStroke - fLowStroke > fStrokeTolerance) {	// while difference exceeds tolerance
		fStroke = fLowStroke + (fHighStroke - fLowStroke) / 2;	// new guess
		bool	bIsErased = OutlineErasesText(fStroke);	// try to erase text
		if (bIsErased) {	// if text was erased
			fHighStroke = fStroke;	// update upper limit
		} else {	// text wasn't erased
			fLowStroke = fStroke;	// update lower limit
		}
	}
	fEraseStroke = fHighStroke;	// update result with optimal stroke
	return true;
}

void CMeltTextProbe::SimplifyText(CString& sText)
{
	// remove whitespace and duplicate characters
	int	nOldLen = sText.GetLength();
	CString	sNewText;
	LPTSTR	pNewText = sNewText.GetBuffer(nOldLen);	// allocate buffer
	int	nNewLen = 0;
	std::bitset<128> aUsed;
	for (int iOldChar = 0; iOldChar < nOldLen; iOldChar++) {
		TCHAR	c = sText[iOldChar];
		if (c == ' ' || c == '\n' || c == '\t')	// if whitespace
			continue;	// skip character
		if (c < 128) {	// if character in ASCII range
			if (!aUsed[c]) {	// if character is unused
				pNewText[nNewLen] = c;	// append to buffer
				nNewLen++;	// bump new character count
				aUsed.set(c);	// mark character used
			}
		} else {	// non-ASCII character
			pNewText[nNewLen] = c;	// append to buffer
			nNewLen++;	// bump new character count
		}
	}
	sNewText.ReleaseBuffer(nNewLen);	// release buffer
	sText = sNewText;	// copy result to caller's string
}

bool CMeltTextProbe::OutlineErasesText(float fOutlineStroke)
{
	// drawing consumes most of the time
	m_pRT->BeginDraw();
	m_pRT->Clear(D2D1::ColorF(0));	// clear to background color
	m_pRT->DrawTextLayout(m_ptText, m_pTextLayout, m_pDrawBrush);	// fill text
	m_pTextLayout->Draw(&fOutlineStroke, this, m_ptText.x, m_ptText.y);	// erase text outline
	m_pRT->EndDraw();
	CComPtr<IWICBitmapLock> pLock;
	WICRect r = {0, 0, m_szBmp.cx, m_szBmp.cy};
	CHECK(m_pWICBmp->Lock(&r, WICBitmapLockRead, &pLock));	// lock bitmap memory
	UINT	cbBufSize = 0;
	UINT	nStride = 0;
	BYTE*	pBufData = NULL;
	pLock->GetStride(&nStride);
	pLock->GetDataPointer(&cbBufSize, &pBufData);
	CSize	sz(m_szBmp);
	for (int y = 0; y < sz.cy; ++y) {	// for each row
		const BYTE* pRow = pBufData + y * nStride;
		for (int x = 0; x < sz.cx; ++x) {	// for each column
			if (pRow[x * 4]) {	// if pixel isn't background color
				return false;	// text wasn't erased by outline
			}
		}
	}
	return true;	// text completely erased by outline
}

HRESULT CMeltTextProbe::DrawGlyphRun(void* pClientDrawingContext, FLOAT fBaselineOriginX, 
	FLOAT fBaselineOriginY, DWRITE_MEASURING_MODE measuringMode, DWRITE_GLYPH_RUN const* pGlyphRun, 
	DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, IUnknown* pClientDrawingEffect)
{
	// drawing context is pointer to outline stroke, cast from void
	float*	pOutlineStroke = static_cast<float*>(pClientDrawingContext);
	CComPtr<ID2D1PathGeometry> pPathGeom;
	CHECK(m_pD2DFactory->CreatePathGeometry(&pPathGeom));	// create path geometry
	CComPtr<ID2D1GeometrySink> pGeomSink;
	CHECK(pPathGeom->Open(&pGeomSink));	// open geometry sink
	const DWRITE_GLYPH_RUN& run = *pGlyphRun;
	CComPtr<IDWriteFontFace> pFontFace = run.fontFace;
	CHECK(pFontFace->GetGlyphRunOutline(run.fontEmSize, run.glyphIndices, run.glyphAdvances, 
		run.glyphOffsets, run.glyphCount, run.isSideways, run.bidiLevel, pGeomSink));	// get outline
	CHECK(pGeomSink->Close());	// close geometry sink
	auto mTranslate(D2D1::Matrix3x2F::Translation(fBaselineOriginX, fBaselineOriginY));
	m_pRT->SetTransform(mTranslate);	// apply origin translation
	m_pRT->DrawGeometry(pPathGeom, m_pBkgndBrush, *pOutlineStroke, m_pStrokeStyle);	// draw outline
	m_pRT->SetTransform(D2D1::Matrix3x2F::Identity());	// remove transform
	return S_OK;
}

bool CMeltTextProbe::WriteBitmap(IWICBitmap* pBitmap, LPCTSTR pszImagePath)
{
	CComPtr<IWICBitmapEncoder> pEncoder;
	CHECK(m_pWICFactory->CreateEncoder(GUID_ContainerFormatTiff, NULL, &pEncoder));	// create encoder
	CComPtr<IWICStream> pStream;
	CHECK(m_pWICFactory->CreateStream(&pStream));	// create WIC stream
	CHECK(pStream->InitializeFromFilename(pszImagePath, GENERIC_WRITE)); // initialize stream
	CHECK(pEncoder->Initialize(pStream, WICBitmapEncoderNoCache));	// initialize encoder with stream
	CComPtr<IWICBitmapFrameEncode>	pFrame;
	CHECK(pEncoder->CreateNewFrame(&pFrame, NULL));	// create WIC frame
	CHECK(pFrame->Initialize(NULL));	// initialize frame encoder
	UINT	uiWidth, uiHeight;
	CHECK(pBitmap->GetSize(&uiWidth, &uiHeight));	// get bitmap dimensions
	CHECK(pFrame->SetSize(uiWidth, uiHeight));	// set desired image dimensions
	WICPixelFormatGUID formatGUID = GUID_WICPixelFormat32bppBGRA;	// pixel format GUID
	CHECK(pFrame->SetPixelFormat(&formatGUID));	// set desired pixel format
	WICRect	rBmp = {0, 0, uiWidth, uiHeight};
	CHECK(pFrame->WriteSource(m_pWICBmp, &rBmp));
	CHECK(pFrame->Commit());	// commit frame to image
	CHECK(pEncoder->Commit());	// commit all image changes and close stream
	return true;
}
