// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      24nov25	initial version

*/

#include "stdafx.h"
#include "Sloganeer.h"
#include "SloganDraw.h"
#include "Easing.h"
#include "D2DOffscreen.h"
#include <algorithm>	// for swap
#define _USE_MATH_DEFINES
#include "math.h"

#define DTF(x) static_cast<float>(x)

#define CHECK(x) { HRESULT hr = x; if (FAILED(hr)) { OnError(hr, __FILE__, __LINE__, __DATE__); return false; }}

void CSloganDraw::TransScroll()
{
	CKD2DRectF	rText;
	CD2DSizeF	szRT(GetTextBounds(rText));
	CD2DSizeF	szText(rText.Size());
	bool	bIncoming = !IsTransOut();
	double	fPhase = EaseInOut(bIncoming, m_fTransProgress, m_fEasing);
	if (bIncoming)	// if incoming transition
		fPhase = fPhase - 1;	// reverse direction
	if (m_iTransType == TT_SCROLL_RL || m_iTransType == TT_SCROLL_BT)	// if reversed
		fPhase = -fPhase;	// negate phase
	CD2DPointF	ptOrigin(0, 0);
	switch (m_iTransType) {
	case TT_SCROLL_LR:
	case TT_SCROLL_RL:
		if (fPhase < 1) {
			ptOrigin.x = DTF(rText.right * fPhase);
		} else {
			ptOrigin.x = DTF((szRT.width - rText.left) * fPhase);
		}
		break;
	case TT_SCROLL_TB:
	case TT_SCROLL_BT:
		if (fPhase < 1) {
			ptOrigin.y = DTF(rText.bottom * fPhase);
		} else {
			ptOrigin.y = DTF((szRT.height - rText.top) * fPhase);
		}
		break;
	default:
		NODEFAULTCASE;	// improper call
	}
	m_pD2DDeviceContext->DrawTextLayout(ptOrigin, m_pTextLayout, m_pDrawBrush); // draw text
}

void CSloganDraw::TransReveal()
{
	CKD2DRectF	rText;
	GetTextBounds(rText);
	CD2DSizeF	szText(rText.Size());
	switch (m_iTransType) {
	case TT_REVEAL_LR:
		{
			float	fOffset = DTF(szText.width * m_fTransProgress);
			if (IsTransOut())	// if outgoing transition
				fOffset -= szText.width;	// start unmasked
			rText.OffsetRect(fOffset, 0);	// offset text mask horizontally
		}
		break;
	case TT_REVEAL_TB:
		{
			float	fOffset = DTF(szText.height * m_fTransProgress);
			if (IsTransOut())	// if outgoing transition
				fOffset -= szText.height;	// start unmasked
			rText.OffsetRect(0, fOffset);	// offset text mask vertically
		}
		break;
	default:
		NODEFAULTCASE;	// improper call
	}
	CD2DPointF	ptOrigin(0, 0);
	m_pD2DDeviceContext->DrawTextLayout(ptOrigin, m_pTextLayout, m_pDrawBrush);	// draw text
	m_pD2DDeviceContext->FillRectangle(rText, m_pBkgndBrush);	// draw text mask
}

double CSloganDraw::Lerp(double a, double b, double t)
{
	return t * b + (1 - t) * a;
}

void CSloganDraw::TransFade()
{
	double	fPhase = GetPhase(GP_INVERT);
	m_pVarBrush->SetColor(D2D1::ColorF(
		DTF(Lerp(m_clrBkgnd.r, m_clrDraw.r, fPhase)),
		DTF(Lerp(m_clrBkgnd.g, m_clrDraw.g, fPhase)),
		DTF(Lerp(m_clrBkgnd.b, m_clrDraw.b, fPhase))
	));
	CD2DPointF	ptOrigin(0, 0);
	m_pD2DDeviceContext->DrawTextLayout(ptOrigin, m_pTextLayout, m_pVarBrush); // draw text
}

void CSloganDraw::TransTypewriter()
{
	if (m_bIsTransStart)	// if start of transition
		m_iTransVariant = rand() & 1;	// coin toss for variant
	if (m_iTransVariant) {	// if variant selected
		TransRandomTypewriter();	// do variant
		return;	// skip default behavior
	}
	UINT	nTextLen = static_cast<UINT>(m_sSlogan.GetLength());
	UINT	nCharsTyped = static_cast<UINT>(Round(nTextLen * m_fTransProgress));
	DWRITE_TEXT_RANGE	trShow = {0, nCharsTyped};
	DWRITE_TEXT_RANGE	trHide = {nCharsTyped, nTextLen - nCharsTyped};
	if (IsTransOut()) {	// if outgoing transition
		std::swap(trShow, trHide);	// swap text ranges
	}
	// Per-character brushes override the default brush passed to DrawTextLayout,
	// and they must be reset to restore normal behavior; see ResetDrawingEffect.
	m_pTextLayout->SetDrawingEffect(m_pDrawBrush, trShow);	// set brush for shown characters
	m_pTextLayout->SetDrawingEffect(m_pBkgndBrush, trHide);	// set brush for hidden characters
	CD2DPointF	ptOrigin(0, 0);
	m_pD2DDeviceContext->DrawTextLayout(ptOrigin, m_pTextLayout, m_pDrawBrush);	// draw text
}

void CSloganDraw::TransRandomTypewriter()
{
	UINT	nTextLen = static_cast<UINT>(m_sSlogan.GetLength());
	if (m_bIsTransStart) {	// if start of transition
		CRandList	rlChar(nTextLen);	// initialize shuffler for entire text
		m_aCharIdx.FastSetSize(nTextLen);	// only reallocate if text too big
		m_aCharIdx.FastRemoveAll();	// reset item count without freeing memory
		for (UINT iChar = 0; iChar < nTextLen; iChar++) {	// for each character
			int	iRandChar = rlChar.GetNext();	// pick a random character
			if (!theApp.IsSpace(m_sSlogan[iRandChar]))	// if non-space character
				m_aCharIdx.Add(iRandChar);	// add character's index to sequence
		}
		m_nCharsTyped = 0;	// reset number of characters typed
	}
	int	nCharsAvail = m_aCharIdx.GetSize();	// number of non-space characters
	UINT	nCharsTyped = static_cast<UINT>(Round(nCharsAvail * m_fTransProgress));
	ID2D1Brush* aBrush[2] = {m_pBkgndBrush, m_pDrawBrush};	// pair of brushes
	int	iBrush = IsTransOut();	// brushes are swapped for outgoing transition
	// for each character that's due to be typed, in sequential order
	for (UINT iChar = m_nCharsTyped; iChar < nCharsTyped; iChar++) {
		int	iMappedChar = m_aCharIdx[iChar];	// get char's index within text
		DWRITE_TEXT_RANGE	tr = {iMappedChar, 1};	// set range for only that char
		m_pTextLayout->SetDrawingEffect(aBrush[!iBrush], tr);	// set brush for char
	}
	m_nCharsTyped = nCharsTyped;	// update count of characters typed
	CD2DPointF	ptOrigin(0, 0);
	m_pD2DDeviceContext->DrawTextLayout(ptOrigin, m_pTextLayout, aBrush[iBrush]);	// draw text
}

void CSloganDraw::TransScale()
{
	float	fPhase = DTF(GetPhase(GP_INVERT | GP_EASING));
	CD2DSizeF	szScale;
	switch (m_iTransType) {
	case TT_SCALE_HORZ:
		szScale = CD2DSizeF(fPhase, 1);	// scale horizontally
		break;
	case TT_SCALE_VERT:
		szScale = CD2DSizeF(1, fPhase);	// scale vertically
		break;
	case TT_SCALE_BOTH:
	case TT_SCALE_SPIN:
		szScale = CD2DSizeF(fPhase, fPhase);	// scale both axes
		break;
	default:
		NODEFAULTCASE;	// improper call
	}
	CD2DSizeF	szRT(m_pD2DDeviceContext->GetSize());	// get render target size
	CD2DPointF	ptCenter(szRT.width / 2, szRT.height / 2);	// scaling center point
	auto mScale(D2D1::Matrix3x2F::Scale(szScale, ptCenter));	// create scaling matrix
	if (m_iTransType == TT_SCALE_SPIN) {	// if spinning too
		auto mRotation(D2D1::Matrix3x2F::Rotation(fPhase * 360, ptCenter));	// create rotation matrix
		mScale = mScale * mRotation;	// apply rotation matrix
	}
	m_pD2DDeviceContext->SetTransform(mScale);	// apply scaling matrix
	CD2DPointF	ptOrigin(0, 0);
	m_pD2DDeviceContext->DrawTextLayout(ptOrigin, m_pTextLayout, m_pDrawBrush);	// draw text
}

void CSloganDraw::InitTiling(const CKD2DRectF& rText)
{
	// ideal tile count is one tile for each frame of transition
	float	fIdealTileCount = DTF(GetFrameRate() * m_fTransDuration);
	// compute tile size: divide text area by ideal tile count and take square root
	CD2DSizeF	szText(rText.Size());
	m_fTileSize = DTF(sqrt(szText.width * szText.height / fIdealTileCount));
	// compute tile layout; round up to ensure text is completely covered
	m_szTileLayout = CSize(
		Round(ceil(szText.width / m_fTileSize)),	// number of tile columns
		Round(ceil(szText.height / m_fTileSize))	// number of tile rows
	);
	// compute offsets for centering tile frame over text
	m_ptTileOffset = CD2DPointF(
		(m_szTileLayout.cx * m_fTileSize - szText.width) / 2,
		(m_szTileLayout.cy * m_fTileSize - szText.height) / 2);
	int	nTiles = m_szTileLayout.cx * m_szTileLayout.cy;	// actual tile count
	m_aTileIdx.SetSize(nTiles);	// allocate tile index array
	CRandList	rlTile(nTiles);	// initialize tile shuffler
	for (int iTile = 0; iTile < nTiles; iTile++) {	// for each tile
		m_aTileIdx[iTile] = rlTile.GetNext();	// set array element to random tile index
	}
}

void CSloganDraw::TransTile()
{
	double	fPhase = GetPhase();
	CKD2DRectF	rText;
	GetTextBounds(rText);
	if (m_bIsTransStart) {	// if start of transition
		InitTiling(rText);	// initialize tiling; only once per transition
	}
	CD2DPointF	ptOrigin(0, 0);
	m_pD2DDeviceContext->DrawTextLayout(ptOrigin, m_pTextLayout, m_pDrawBrush);	// draw text
	int	nTiles = Round(m_aTileIdx.GetSize() * fPhase);	// number of tiles to draw
	for (int iTile = 0; iTile < nTiles; iTile++) {	// for each drawn tile
		int	nTileIdx = m_aTileIdx[iTile];	// get tile's randomized index
		int iRow = nTileIdx / m_szTileLayout.cx;	// compute tile row from index
		int	iCol = nTileIdx % m_szTileLayout.cx;	// compute tile column from index
		CD2DPointF	ptOrg(	// compute tile's origin
			rText.left + m_fTileSize * iCol - m_ptTileOffset.x, 
			rText.top + m_fTileSize * iRow - m_ptTileOffset.y
		);
		CKD2DRectF	rTile(	// compute tile's rectangle
			ptOrg.x, ptOrg.y,
			ptOrg.x + m_fTileSize, ptOrg.y + m_fTileSize
		);
		rTile.InflateRect(1, 1);	// add a pixel to ensure tiles overlap
		m_pD2DDeviceContext->FillRectangle(rTile, m_pBkgndBrush);	// draw tile
	}
}

bool CSloganDraw::TransConverge()
{
	if (!MakeCharToLineTable())
		return false;
	m_bIsGlyphRising = false;
	m_iGlyphLine = 0;
	CHECK(m_pTextLayout->Draw(0, this, 0, 0));
	return true;
}

void CSloganDraw::TransConvergeHorz(CD2DPointF ptBaselineOrigin, DWRITE_MEASURING_MODE measuringMode, 
	DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, DWRITE_GLYPH_RUN const* pGlyphRun)
{
	// odd lines slide left and even lines slide right, or vice versa
	double	fPhase = GetPhase(GP_EASING);	// apply easing
	CKD2DRectF	rText;
	CD2DSizeF	szRT = GetTextBounds(rText);
	float	fSlideSpan = DTF(max(szRT.width - rText.left, rText.right) * fPhase);
	UINT	nGlyphs = pGlyphRun->glyphCount;
	if (m_aLineMetrics.GetSize() == 1) {	// if single line
		CD2DSizeF	szText = rText.Size();
		CKD2DRectF	rClip(CD2DPointF(0, rText.top), CD2DSizeF(szRT.width, szText.height / 2));
		for (int iStrip = 0; iStrip < 2; iStrip++) {	// for each of two horizontal strips
			if (iStrip)	// if second strip
				rClip.OffsetRect(0, szText.height / 2);
			for (UINT iGlyph = 0; iGlyph < nGlyphs; iGlyph++) {	// for each glyph
				float	fOffset = fSlideSpan;
				if (iStrip)	// if second strip
					fOffset = -fOffset * 2;	// opposite slide
				m_aGlyphOffset[iGlyph].advanceOffset += fOffset;
			}
			m_pD2DDeviceContext->PushAxisAlignedClip(rClip, D2D1_ANTIALIAS_MODE_ALIASED);
			m_pD2DDeviceContext->DrawGlyphRun(ptBaselineOrigin, pGlyphRun, m_pDrawBrush, measuringMode);
			m_pD2DDeviceContext->PopAxisAlignedClip();
		}
	} else {	// multiple lines
		for (UINT iGlyph = 0; iGlyph < nGlyphs; iGlyph++) {	// for each glyph
			int	iChar = pGlyphRunDescription->textPosition + iGlyph;
			int	iLine = m_aCharToLine[iChar];
			float	fOffset = fSlideSpan;
			if (iLine & 1)	// if odd line
				fOffset = -fOffset;	// opposite slide
			m_aGlyphOffset[iGlyph].advanceOffset += fOffset;
		}
		m_pD2DDeviceContext->DrawGlyphRun(ptBaselineOrigin, pGlyphRun, m_pDrawBrush, measuringMode);
	}
}

void CSloganDraw::TransConvergeVert(CD2DPointF ptBaselineOrigin, DWRITE_MEASURING_MODE measuringMode, 
	DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, DWRITE_GLYPH_RUN const* pGlyphRun)
{
	// odd characters fall and even characters rise, or vice versa
	double	fPhase = GetPhase(GP_EASING);	// apply easing
	CKD2DRectF	rText;
	CD2DSizeF	szRT = GetTextBounds(rText);
	float	fSlideSpan = DTF(max(szRT.height - rText.top, rText.bottom) * fPhase);
	UINT	nGlyphs = pGlyphRun->glyphCount;
	for (UINT iGlyph = 0; iGlyph < nGlyphs; iGlyph++) {	// for each glyph
		int	iChar = pGlyphRunDescription->textPosition + iGlyph;
		int	iLine = m_aCharToLine[iChar];
		if (iLine != m_iGlyphLine) {	// if line index changed
			m_bIsGlyphRising = false;	// reset alternation state
			m_iGlyphLine = iLine;	// update cached value
		}
		float	fOffset = fSlideSpan;
		if (!theApp.IsSpace(pGlyphRunDescription->string[iGlyph])) {	// exclude spaces
			if (m_bIsGlyphRising)	// if glyph rises
				fOffset = -fOffset;	// negate offset
			m_bIsGlyphRising ^= 1;	// alternate
		}
		m_aGlyphOffset[iGlyph].ascenderOffset += fOffset;
	}
	m_pD2DDeviceContext->DrawGlyphRun(ptBaselineOrigin, pGlyphRun, m_pDrawBrush, measuringMode);
}

bool CSloganDraw::GetLineMetrics()
{
	int	nLines = m_textMetrics.lineCount;
	m_aLineMetrics.SetSize(nLines);	// allocate line metrics destination array
	UINT	nActualLines;	// get line metrics from text layout
	CHECK(m_pTextLayout->GetLineMetrics(m_aLineMetrics.GetData(), nLines, &nActualLines));
	return nActualLines == nLines;
}

bool CSloganDraw::MakeCharToLineTable()
{
	GetLineMetrics();
	m_aCharToLine.SetSize(m_sSlogan.GetLength());	// allocate character-to-line index array
	int	nPos = 0;
	int	nLines = m_textMetrics.lineCount;
	for (int iLine = 0; iLine < nLines; iLine++) {	// for each line of text
		int	nLineLen = m_aLineMetrics[iLine].length;	// get line length from metrics
		for (int iChar = nPos; iChar < nPos + nLineLen; iChar++) {	// for each of line's characters
			m_aCharToLine[iChar] = iLine;	// store line index
		}
		nPos += nLineLen;	// bump position by line length
	}
	return true;
}

bool CSloganDraw::TransMelt()
{
	if (m_bIsTransStart) {	// if start of transition
		if (m_aMeltStroke[m_iSlogan]) {	// if cached melt stroke available
			m_fMeltMaxStroke = m_aMeltStroke[m_iSlogan];	// use cached value
		} else {	// melt stroke isn't cached
			MeasureMeltStroke();	// find appropriate maximum outline stroke
		}
	}
	// if pausing between slogans, and outgoing transition complete
	if (m_nPauseDuration && IsTransOut() && m_fTransProgress >= 1)
		return true;	// avoid potentially showing scraps of unerased text while paused
	m_pD2DDeviceContext->DrawTextLayout(CD2DPointF(0, 0), m_pTextLayout, m_pDrawBrush);	// fill text
	CHECK(m_pTextLayout->Draw(0, this, 0, 0));	// erase text outline
	return true;
}

bool CSloganDraw::TransMelt(CD2DPointF ptBaselineOrigin, DWRITE_MEASURING_MODE measuringMode, 
	DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, DWRITE_GLYPH_RUN const* pGlyphRun)
{
	double	fPhase = GetPhase();
	CComPtr<ID2D1PathGeometry> pPathGeom;
	CHECK(m_pD2DFactory->CreatePathGeometry(&pPathGeom));
	CComPtr<ID2D1GeometrySink> pGeomSink;
	CHECK(pPathGeom->Open(&pGeomSink));
	const DWRITE_GLYPH_RUN& run = *pGlyphRun;
	CComPtr<IDWriteFontFace> pFontFace = run.fontFace;
	CHECK(pFontFace->GetGlyphRunOutline(run.fontEmSize, run.glyphIndices, run.glyphAdvances, 
		run.glyphOffsets, run.glyphCount, run.isSideways, run.bidiLevel, pGeomSink));
	CHECK(pGeomSink->Close());
	auto mTranslate(D2D1::Matrix3x2F::Translation(ptBaselineOrigin.x, ptBaselineOrigin.y));
	m_pD2DDeviceContext->SetTransform(mTranslate);
	float	fStroke = DTF(m_fMeltMaxStroke * fPhase);
	m_pD2DDeviceContext->DrawGeometry(pPathGeom, m_pBkgndBrush, fStroke, m_pStrokeStyle);
	m_pD2DDeviceContext->SetTransform(D2D1::Matrix3x2F::Identity());	// remove transform
	return S_OK;
}

bool CSloganDraw::LaunchMeltWorker()
{
	CD2DPointF	ptDPI;
	m_pD2DDeviceContext->GetDpi(&ptDPI.x, &ptDPI.y);
	m_aMeltStroke.SetSize(m_aSlogan.GetSize());	// allocate destination stroke array
	return m_thrMeltWorker.Create(m_aSlogan, ptDPI, m_aMeltStroke);	// launch worker thread
}

bool CSloganDraw::MeasureMeltStroke()
{
	CD2DPointF	ptDPI;
	m_pD2DDeviceContext->GetDpi(&ptDPI.x, &ptDPI.y);
	CMeltProbe	probe(m_pD2DFactory, m_pDWriteFactory, m_pStrokeStyle);
	if (!probe.Create(m_sSlogan, m_sFontName, m_fFontSize, m_nFontWeight, ptDPI, m_fMeltMaxStroke))
		return false;
#if 0	// non-zero to compare text bitmap with screen text
	CKD2DRectF	rText;
	CD2DSizeF	szScrRT = GetTextBounds(rText);
	CD2DSizeF	szText(rText.Size());
	CComPtr<ID2D1Bitmap> pTempBmp;
	CHECK(m_pD2DDeviceContext->CreateBitmapFromWicBitmap(probe.GetBitmap(), NULL, &pTempBmp));
	CSize	szBmp(probe.GetBitmapSize());
	CKD2DRectF	rOutBmp(
		CD2DPointF((szScrRT.width - float(szBmp.cx)) / 2, (szScrRT.height - float(szBmp.cy)) / 2), 
		CD2DSizeF(float(szBmp.cx), float(szBmp.cy)));
	m_pD2DDeviceContext->DrawBitmap(pTempBmp, rOutBmp);
	// draw reference text
	m_pVarBrush->SetColor(D2D1::ColorF(1, 0, 0));
	m_pD2DDeviceContext->DrawTextLayout(CD2DPointF(0, 0), m_pTextLayout, m_pVarBrush);
#endif
	return true;
}

bool CSloganDraw::TransElevator()
{
	CHECK(m_pTextLayout->Draw(0, this, 0, 0));
	return true;
}

void CSloganDraw::TransElevator(CD2DPointF ptBaselineOrigin, DWRITE_MEASURING_MODE measuringMode, 
	DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, DWRITE_GLYPH_RUN const* pGlyphRun)
{
	double	fPhase = GetPhase();
	m_pD2DDeviceContext->DrawGlyphRun(ptBaselineOrigin, pGlyphRun, m_pDrawBrush, measuringMode);
	CGlyphIter	iterGlyph(ptBaselineOrigin, pGlyphRun);
	CKD2DRectF	rGlyph;
	UINT	iGlyph;
	while (iterGlyph.GetNext(iGlyph, rGlyph)) {	// for each glyph
		// for space testing, don't assume one-to-one mapping of glyphs to characters,
		// as that breaks in RTL languages; safer to test for a zero width bounding box
		if (rGlyph.Width() > m_fSpaceWidth) {	// if non-space glyph
			rGlyph.InflateRect(AA_MARGIN, AA_MARGIN);	// add antialiasing margin
			float	fGlyphWidth = rGlyph.Width();
			float	fDoorWidth = DTF((fGlyphWidth + AA_MARGIN) * fPhase / 2);
			float	fDoorLeft = rGlyph.left + fDoorWidth;
			float	fDoorRight = rGlyph.right - fDoorWidth;
			CD2DRectF	rDoor(rGlyph.left, rGlyph.top, fDoorLeft, rGlyph.bottom);
			m_pD2DDeviceContext->FillRectangle(rDoor, m_pBkgndBrush);	// left door
			rDoor.left = fDoorRight;
			rDoor.right = rGlyph.right;
			m_pD2DDeviceContext->FillRectangle(rDoor, m_pBkgndBrush);	// right door
		}
	}
}

bool CSloganDraw::TransClock()
{
	CHECK(m_pTextLayout->Draw(0, this, 0, 0));
	return true;
}

bool CSloganDraw::TransClock(CD2DPointF ptBaselineOrigin, DWRITE_MEASURING_MODE measuringMode, 
	DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, DWRITE_GLYPH_RUN const* pGlyphRun)
{
	double	fPhase = GetPhase();
	CGlyphIter	iterGlyph(ptBaselineOrigin, pGlyphRun);
	CKD2DRectF	rGlyph;
	UINT	iGlyph;
	float	fMaxGlyphSize = 0;
	while (iterGlyph.GetNext(iGlyph, rGlyph)) {	// for each glyph
		CD2DSizeF	szGlyph(rGlyph.Size());
		float	fGlyphSize = max(szGlyph.width, szGlyph.height);
		if (fGlyphSize > fMaxGlyphSize)
			fMaxGlyphSize = fGlyphSize;
	}
	float	fRadius = DTF(fMaxGlyphSize * M_SQRT2 / 2);
	CComPtr<ID2D1PathGeometry> pPathGeom;
	CHECK(m_pD2DFactory->CreatePathGeometry(&pPathGeom));
	CComPtr<ID2D1GeometrySink> pGeomSink;
	CHECK(pPathGeom->Open(&pGeomSink));
	AddPieWedge(pGeomSink, CD2DPointF(0, 0), CD2DSizeF(fRadius, fRadius), 180, DTF(fPhase));
	CHECK(pGeomSink->Close());
	m_pD2DDeviceContext->DrawGlyphRun(ptBaselineOrigin, pGlyphRun, m_pDrawBrush, measuringMode);
	iterGlyph.Reset();	// reset glyph iterator, as we reuse it below
	while (iterGlyph.GetNext(iGlyph, rGlyph)) {	// for each glyph
		if (rGlyph.Width() > m_fSpaceWidth) {	// if non-space glyph
			rGlyph.InflateRect(AA_MARGIN, AA_MARGIN);	// add antialiasing margin
			CD2DPointF	ptCenter(rGlyph.CenterPoint());
			auto mTranslate(D2D1::Matrix3x2F::Translation(ptCenter.x, ptCenter.y));
			m_pD2DDeviceContext->PushAxisAlignedClip(rGlyph, D2D1_ANTIALIAS_MODE_ALIASED);
			m_pD2DDeviceContext->SetTransform(mTranslate);
			m_pD2DDeviceContext->FillGeometry(pPathGeom, m_pBkgndBrush);
			m_pD2DDeviceContext->SetTransform(D2D1::Matrix3x2F::Identity());
			m_pD2DDeviceContext->PopAxisAlignedClip();
		}
	}
	return true;
}

bool CSloganDraw::TransSkew()
{
	CHECK(m_pTextLayout->Draw(0, this, 0, 0));
	return true;
}

void CSloganDraw::TransSkew(CD2DPointF ptBaselineOrigin, DWRITE_MEASURING_MODE measuringMode, 
	DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, DWRITE_GLYPH_RUN const* pGlyphRun)
{
	double	fPhase = !IsTransOut() ? (1 - m_fTransProgress) : -m_fTransProgress;
	auto mSkew(D2D1::Matrix3x2F::Skew(DTF(90 * fPhase), 0, ptBaselineOrigin));
	m_pD2DDeviceContext->SetTransform(mSkew);
	m_pD2DDeviceContext->DrawGlyphRun(ptBaselineOrigin, pGlyphRun, m_pDrawBrush, measuringMode);
	m_pD2DDeviceContext->SetTransform(D2D1::Matrix3x2F::Identity());
}

bool CSloganDraw::TransExplode()
{
	if (m_bIsTransStart) {	// if start of transition
		m_triSink.OnStartTrans(m_pD2DDeviceContext->GetSize());
	}
	m_triSink.OnDraw();
	CHECK(m_pTextLayout->Draw(0, this, 0, 0));
	return true;
}

bool CSloganDraw::TransExplode(CD2DPointF ptBaselineOrigin, DWRITE_MEASURING_MODE measuringMode, 
	DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, DWRITE_GLYPH_RUN const* pGlyphRun)
{
	const DWRITE_GLYPH_RUN& run = *pGlyphRun;
	CGlyphIter	iterGlyph(ptBaselineOrigin, pGlyphRun);
	CKD2DRectF	rGlyph;
	UINT	iTemp;
	if (m_bIsTransStart) {	// if start of transition
		m_triSink.SetRunBidiLevel(pGlyphRun->bidiLevel);	// pass run's bidi level to sink
		for (UINT iGlyph = 0; iGlyph < run.glyphCount; iGlyph++) {	// for each glyph
			float fOriginX = iterGlyph.GetOrigin().x;	// before GetNext changes it
			iterGlyph.GetNext(iTemp, rGlyph);
			CComPtr<ID2D1PathGeometry> pPathGeom;
			CHECK(m_pD2DFactory->CreatePathGeometry(&pPathGeom));	// create path geometry
			CComPtr<ID2D1GeometrySink> pGeomSink;
			CHECK(pPathGeom->Open(&pGeomSink));	// open geometry sink
			CComPtr<IDWriteFontFace> pFontFace = run.fontFace;
			CHECK(pFontFace->GetGlyphRunOutline(run.fontEmSize,	// get glyph run's outline
				run.glyphIndices + iGlyph, run.glyphAdvances + iGlyph, 
				run.glyphOffsets + iGlyph, 1, run.isSideways, run.bidiLevel & 1, pGeomSink));
			CHECK(pGeomSink->Close());	// close geometry sink
			CHECK(m_triSink.TessellateGlyph(ptBaselineOrigin, rGlyph, pPathGeom));	// tessellate glyph
		}
		iterGlyph.Reset();	// reset glyph iterator, as we reuse it below
	}
	double	fPhase = GetPhase();
	fPhase = EaseIn(fPhase, 1);	// easing entire trajectory looks best
	double	fRad = m_triSink.GetTravel() * fPhase;
	for (UINT iGlyph = 0; iGlyph < run.glyphCount; iGlyph++) {	// for each glyph
		float fOriginX = iterGlyph.GetOrigin().x;	// before GetNext changes it
		iterGlyph.GetNext(iTemp, rGlyph);
		CComPtr<ID2D1PathGeometry> pTriGeom;
		CHECK(m_pD2DFactory->CreatePathGeometry(&pTriGeom));
		CComPtr<ID2D1GeometrySink> pTriSink;
		CHECK(pTriGeom->Open(&pTriSink));	// open geometry sink
		int	iStartTri, iEndTri;	// range of triangles; end is exclusive
		m_triSink.GetNextGlyph(iStartTri, iEndTri);	// get glyph's triangles
		for (int iTri = iStartTri; iTri < iEndTri; iTri++) {	// for each triangle
			// radiate triangles in proportion to transition progress
			const CTriangleSink::GLYPH_TRIANGLE&	gt = m_triSink.GetTriangle(iTri);
			float	x = DTF(sin(gt.fAngle) * fRad);
			float	y = DTF(cos(gt.fAngle) * fRad);
			D2D1_TRIANGLE	t = gt;
			t.point1.x += x; t.point2.x += x; t.point3.x += x;
			t.point1.y += y; t.point2.y += y; t.point3.y += y;
			// add triangle to geometry sink
			pTriSink->BeginFigure(t.point1, D2D1_FIGURE_BEGIN_FILLED);	// begin figure
			pTriSink->AddLines(&t.point2, 2);	// add triangle's other two points
			pTriSink->EndFigure(D2D1_FIGURE_END_CLOSED);	// end figure
		}
		CHECK(pTriSink->Close());	// close geometry sink
		auto mTranslate(D2D1::Matrix3x2F::Translation(fOriginX, ptBaselineOrigin.y));
		m_pD2DDeviceContext->SetTransform(mTranslate);	// translate to left edge and baseline
		m_pD2DDeviceContext->FillGeometry(pTriGeom, m_pDrawBrush);	// fill geometry
		m_pD2DDeviceContext->SetTransform(D2D1::Matrix3x2F::Identity());
	}
	return true;
}
