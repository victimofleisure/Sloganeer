// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      24nov25	initial version
		01		25nov25	decouple typewriter transitions
        02      27nov25	add submarine transition
		03		28nov25	add symmetrical easing
		04		29nov25	refactor reveal to use clipping
		05		30nov25	add eraser to handle transparent background
		06		01dec25	add transparent background special cases
		07		15dec25	add tumble transition
		08		19dec25	add vertical displacement to tumble
		09		20dec25	add iris transition
		10		21dec25	refactor elevator and clock to use clipping
		11		27dec25	add glyph run callback member function pointer
		12		28dec25	merge typewriter effects
		13		29dec25	add blur transition

*/

#include "stdafx.h"
#include "Sloganeer.h"
#include "SloganDraw.h"
#include "Easing.h"
#include "D2DOffscreen.h"
#include <algorithm>	// for swap
#define _USE_MATH_DEFINES
#include "math.h"
#include "DPoint.h"

#define DTF(x) static_cast<float>(x)

#define CHECK(x) { HRESULT hr = x; if (FAILED(hr)) { OnError(hr, __FILE__, __LINE__, __DATE__); return false; }}

void CSloganDraw::TransScroll()
{
	CKD2DRectF	rText;
	CKD2DSizeF	szRT(GetTextBounds(rText));
	CKD2DSizeF	szText(rText.Size());
	bool	bIncoming = !IsTransOut();
	double	fPhase = EaseInOrOut(bIncoming, m_fTransProgress, m_fEasing);
	if (bIncoming)	// if incoming transition
		fPhase = fPhase - 1;	// reverse direction
	if (m_iTransType == TT_SCROLL_RL || m_iTransType == TT_SCROLL_BT)	// if reversed
		fPhase = -fPhase;	// negate phase
	CKD2DPointF	ptOrigin(0, 0);
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
	CKD2DSizeF	szText(rText.Size());
	float	fOffset;
	switch (m_iTransType) {
	case TT_REVEAL_LR:
		fOffset = DTF(rText.left + szText.width * m_fTransProgress);
		if (IsTransOut()) {	// if outgoing transition
			rText.left = fOffset;	// offset left edge
		} else {	// incoming transition
			rText.right = fOffset;	// offset right edge
		}
		break;
	case TT_REVEAL_TB:
		fOffset = DTF(rText.top + szText.height * m_fTransProgress);
		if (IsTransOut()) {	// if outgoing transition
			rText.top = fOffset;	// offset top edge
		} else {	// incoming transition
			rText.bottom = fOffset;	// offset bottom edge
		}
		break;
	default:
		NODEFAULTCASE;	// improper call
	}
	CKD2DPointF	ptOrigin(0, 0);
	m_pD2DDeviceContext->PushAxisAlignedClip(rText, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
	m_pD2DDeviceContext->DrawTextLayout(ptOrigin, m_pTextLayout, m_pDrawBrush);	// draw text
	m_pD2DDeviceContext->PopAxisAlignedClip();
}

void CSloganDraw::TransFade()
{
	double	fPhase = GetPhase(GP_INVERT);
	D2D1::ColorF	clr(0);
	if (m_bTransparentBkgnd) {	// if transparent background
		// use draw color RGB values, scaling alpha by phase
		clr = D2D1::ColorF(
			m_clrDraw.r, m_clrDraw.g, m_clrDraw.b, DTF(m_clrDraw.a * fPhase));
	} else {	// linear interpolation with background color
		clr = D2D1::ColorF(
			DTF(Lerp(m_clrBkgnd.r, m_clrDraw.r, fPhase)),
			DTF(Lerp(m_clrBkgnd.g, m_clrDraw.g, fPhase)),
			DTF(Lerp(m_clrBkgnd.b, m_clrDraw.b, fPhase))
		);
	}
	m_pVarBrush->SetColor(clr);
	CKD2DPointF	ptOrigin(0, 0);
	m_pD2DDeviceContext->DrawTextLayout(ptOrigin, m_pTextLayout, m_pVarBrush); // draw text
}

void CSloganDraw::TransTypewriter()
{
	// setting the drawing effect is costly, so only do it when a character is typed
	UINT	nTextLen = static_cast<UINT>(m_sSlogan.GetLength());
	if (m_bIsTransStart) {	// if start of transition
		m_aCharIdx.FastSetSize(nTextLen);	// only reallocate if text too big
		m_aCharIdx.FastRemoveAll();	// reset item count without freeing memory
		if (m_iTransType == TT_RAND_TYPE) {	// if typing in random order
			CRandList	rlChar(nTextLen);	// initialize shuffler for entire text
			for (UINT iChar = 0; iChar < nTextLen; iChar++) {	// for each character
				int	iRandChar = rlChar.GetNext();	// pick a random character
				if (!theApp.IsSpace(m_sSlogan[iRandChar]))	// if non-space character
					m_aCharIdx.Add(iRandChar);	// add character's index to sequence
			}
		} else {	// typing in sequential order
			for (UINT iChar = 0; iChar < nTextLen; iChar++) {	// for each character
				m_aCharIdx.Add(iChar);	// add character's index to sequence
			}
		}
		m_nCharsTyped = 0;	// reset number of characters typed
	}
	int	nCharsAvail = m_aCharIdx.GetSize();	// number of non-space characters
	UINT	nCharsTyped = static_cast<UINT>(Round(nCharsAvail * m_fTransProgress));
	ID2D1Brush* aBrush[2] = {m_pBkgndBrush, m_pDrawBrush};	// pair of brushes
	int	iBrush = IsTransOut();	// brushes are swapped for outgoing transition
	// for each character that's due to be typed, in sequential order
	for (UINT iChar = m_nCharsTyped; iChar < nCharsTyped; iChar++) {
		UINT	iMappedChar = m_aCharIdx[iChar];	// get char's index within text
		DWRITE_TEXT_RANGE	tr = {iMappedChar, 1};	// set range for only that char
		m_pTextLayout->SetDrawingEffect(aBrush[!iBrush], tr);	// set brush for char
	}
	m_nCharsTyped = nCharsTyped;	// update count of characters typed
	CKD2DPointF	ptOrigin(0, 0);
	m_pD2DDeviceContext->DrawTextLayout(ptOrigin, m_pTextLayout, aBrush[iBrush]);	// draw text
}

void CSloganDraw::TransScale()
{
	float	fPhase = DTF(GetPhase(GP_INVERT | GP_EASING));
	CKD2DSizeF	szScale;
	switch (m_iTransType) {
	case TT_SCALE_HORZ:
		szScale = CKD2DSizeF(fPhase, 1);	// scale horizontally
		break;
	case TT_SCALE_VERT:
		szScale = CKD2DSizeF(1, fPhase);	// scale vertically
		break;
	case TT_SCALE_BOTH:
	case TT_SCALE_SPIN:
		szScale = CKD2DSizeF(fPhase, fPhase);	// scale both axes
		break;
	default:
		NODEFAULTCASE;	// improper call
	}
	CKD2DSizeF	szRT(m_pD2DDeviceContext->GetSize());	// get render target size
	CKD2DPointF	ptCenter(szRT.width / 2, szRT.height / 2);	// scaling center point
	auto mScale(D2D1::Matrix3x2F::Scale(szScale, ptCenter));	// create scaling matrix
	if (m_iTransType == TT_SCALE_SPIN) {	// if spinning too
		auto mRotation(D2D1::Matrix3x2F::Rotation(fPhase * 360, ptCenter));	// create rotation matrix
		mScale = mScale * mRotation;	// apply rotation matrix
	}
	m_pD2DDeviceContext->SetTransform(mScale);	// apply scaling matrix
	CKD2DPointF	ptOrigin(0, 0);
	m_pD2DDeviceContext->DrawTextLayout(ptOrigin, m_pTextLayout, m_pDrawBrush);	// draw text
}

void CSloganDraw::InitTiling(const CKD2DRectF& rText)
{
	// ideal tile count is one tile for each frame of transition
	float	fIdealTileCount = DTF(GetFrameRate() * m_fStateDur);
	// compute tile size: divide text area by ideal tile count and take square root
	CKD2DSizeF	szText(rText.Size());
	m_fTileSize = DTF(sqrt(szText.width * szText.height / fIdealTileCount));
	// compute tile layout; round up to ensure text is completely covered
	m_szTileLayout = CSize(
		Round(ceil(szText.width / m_fTileSize)),	// number of tile columns
		Round(ceil(szText.height / m_fTileSize))	// number of tile rows
	);
	// compute offsets for centering tile frame over text
	m_ptTileOffset = CKD2DPointF(
		(m_szTileLayout.cx * m_fTileSize - szText.width) / 2,
		(m_szTileLayout.cy * m_fTileSize - szText.height) / 2);
	int	nTiles = m_szTileLayout.cx * m_szTileLayout.cy;	// actual tile count
	m_aTileIdx.SetSize(nTiles);	// allocate tile index array
	CRandList	rlTile(nTiles);	// initialize tile shuffler
	for (int iTile = 0; iTile < nTiles; iTile++) {	// for each tile
		m_aTileIdx[iTile] = rlTile.GetNext();	// set array element to random tile index
	}
	if (m_bTransparentBkgnd) {	// if transparent background
		CreateEraser(CKD2DSizeF(m_fTileSize, m_fTileSize));	// create eraser
	}
}

void CSloganDraw::TransRandTile()
{
	double	fPhase = GetPhase();
	CKD2DRectF	rText;
	GetTextBounds(rText);
	if (m_bIsTransStart) {	// if start of transition
		InitTiling(rText);	// initialize tiling; only once per transition
	}
	CKD2DPointF	ptOrigin(0, 0);
	m_pD2DDeviceContext->DrawTextLayout(ptOrigin, m_pTextLayout, m_pDrawBrush);	// draw text
	int	nTiles = Round(m_aTileIdx.GetSize() * fPhase);	// number of tiles to draw
	for (int iTile = 0; iTile < nTiles; iTile++) {	// for each drawn tile
		int	nTileIdx = m_aTileIdx[iTile];	// get tile's randomized index
		int iRow = nTileIdx / m_szTileLayout.cx;	// compute tile row from index
		int	iCol = nTileIdx % m_szTileLayout.cx;	// compute tile column from index
		CKD2DPointF	ptOrg(	// compute tile's origin
			rText.left + m_fTileSize * iCol - m_ptTileOffset.x, 
			rText.top + m_fTileSize * iRow - m_ptTileOffset.y
		);
		CKD2DRectF	rTile(	// compute tile's rectangle
			ptOrg.x, ptOrg.y,
			ptOrg.x + m_fTileSize, ptOrg.y + m_fTileSize
		);
		rTile.InflateRect(1, 1);	// add a pixel to ensure tiles overlap
		if (m_bTransparentBkgnd) {	// if transparent background
			EraseBackground(rTile.TopLeft());	// erase tile
		} else {	// draw with background color
			m_pD2DDeviceContext->FillRectangle(rTile, m_pBkgndBrush);	// draw tile
		}
	}
}

bool CSloganDraw::TransConverge()
{
	if (!MakeCharToLineTable())
		return false;
	m_bIsGlyphRising = false;
	m_iGlyphLine = 0;
	if (m_iTransType == TT_CONVERGE_HORZ)
		m_pGlyphRunCB = &CSloganDraw::TransConvergeHorz;
	else
		m_pGlyphRunCB = &CSloganDraw::TransConvergeVert;
	CHECK(m_pTextLayout->Draw(0, this, 0, 0));
	return true;
}

bool CSloganDraw::TransConvergeHorz(CKD2DPointF ptBaselineOrigin, DWRITE_MEASURING_MODE measuringMode, 
	DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, DWRITE_GLYPH_RUN const* pGlyphRun)
{
	// odd lines slide left and even lines slide right, or vice versa
	double	fPhase = GetPhase(GP_EASING);	// apply easing
	CKD2DRectF	rText;
	CKD2DSizeF	szRT = GetTextBounds(rText);
	float	fSlideSpan = DTF(max(szRT.width - rText.left, rText.right) * fPhase);
	UINT	nGlyphs = pGlyphRun->glyphCount;
	if (m_aLineMetrics.GetSize() == 1) {	// if single line
		CKD2DSizeF	szText = rText.Size();
		CKD2DRectF	rClip(CKD2DPointF(0, rText.top), CKD2DSizeF(szRT.width, szText.height / 2));
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
	return true;
}

bool CSloganDraw::TransConvergeVert(CKD2DPointF ptBaselineOrigin, DWRITE_MEASURING_MODE measuringMode, 
	DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, DWRITE_GLYPH_RUN const* pGlyphRun)
{
	// odd characters fall and even characters rise, or vice versa
	double	fPhase = GetPhase(GP_EASING);	// apply easing
	CKD2DRectF	rText;
	CKD2DSizeF	szRT = GetTextBounds(rText);
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
	return true;
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
		// if current slogan index is valid and cached melt stroke is available
		if (m_iSlogan >= 0 && m_aMeltStroke[m_iSlogan]) {
			m_fMeltMaxStroke = m_aMeltStroke[m_iSlogan];	// use cached value
		} else {	// melt stroke isn't cached, or invalid slogan index
			// The melt probe is slow enough that we might drop frames, which
			// is why it normally runs from a worker thread, but in this case
			// we need the stroke to start the transition and it's not cached,
			// so there's no choice but to do the probe in the render thread.
			MeasureMeltStroke();	// find appropriate maximum outline stroke
		}
	}
	m_pD2DDeviceContext->DrawTextLayout(CKD2DPointF(0, 0), m_pTextLayout, m_pDrawBrush);	// fill text
	m_pGlyphRunCB = &CSloganDraw::TransMelt;
	CHECK(m_pTextLayout->Draw(0, this, 0, 0));	// erase text outline
	return true;
}

bool CSloganDraw::TransMelt(CKD2DPointF ptBaselineOrigin, DWRITE_MEASURING_MODE measuringMode, 
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
	float	fStroke = DTF(m_fMeltMaxStroke * fPhase);
	if (m_bTransparentBkgnd) {	// if transparent background
		CKD2DRectF	rRun;	// bounds of all of run's glyphs, in world coords
		GetRunBounds(rRun, ptBaselineOrigin, pGlyphRun);
		rRun.InflateRect(AA_MARGIN, AA_MARGIN);	// account for DrawImage antialiasing to avoid unerased sliver
		CreateEraser(rRun.Size(), 0);	// create eraser bitmap and clear it to zero alpha
		CComPtr<ID2D1Image>	pOldTarget;	// smart pointer is required due to reference counting
		m_pD2DDeviceContext->GetTarget(&pOldTarget);	// increments target's reference count
		m_pD2DDeviceContext->SetTarget(m_pEraserBitmap);	// set target to eraser bitmap
		m_pVarBrush->SetColor(D2D1::ColorF(0, 0, 0, 1));	// draw with full-strength erasure
		// convert baseline origin from world coords to eraser bitmap local coords
		D2D1_POINT_2F	ptOffset = DPoint(ptBaselineOrigin) - rRun.TopLeft();
		auto mTranslate(D2D1::Matrix3x2F::Translation(ptOffset.x, ptOffset.y));
		m_pD2DDeviceContext->SetTransform(mTranslate);
		// draw glyph outlines in eraser bitmap; outlines are full alpha, rest is zero alpha
		m_pD2DDeviceContext->DrawGeometry(pPathGeom, m_pVarBrush, fStroke, m_pStrokeStyle);
		m_pD2DDeviceContext->SetTarget(pOldTarget);	// restore previous target
		ResetTransform(m_pD2DDeviceContext);	// remove transform
		EraseBackground(rRun.TopLeft());	// erase glyph outlines via destination out blending
	} else {	// draw outline with background color
		auto mTranslate(D2D1::Matrix3x2F::Translation(ptBaselineOrigin.x, ptBaselineOrigin.y));
		m_pD2DDeviceContext->SetTransform(mTranslate);
		m_pD2DDeviceContext->DrawGeometry(pPathGeom, m_pBkgndBrush, fStroke, m_pStrokeStyle);
		ResetTransform(m_pD2DDeviceContext);	// remove transform
	}
	return S_OK;
}

bool CSloganDraw::LaunchMeltWorker(int iSelSlogan)
{
	m_thrMeltWorker.Destroy();	// wait for worker thread to exit
	CKD2DPointF	ptDPI;
	m_pD2DDeviceContext->GetDpi(&ptDPI.x, &ptDPI.y);
	// The Create method resizes the melt stroke array to match the number of
	// slogans, and also zeros all or only one of the stroke array's elements,
	// depending on whether a selected slogan is specified (iSelSlogan >= 0).
	return m_thrMeltWorker.Create(m_aSlogan, ptDPI, m_aMeltStroke, iSelSlogan);	// launch worker thread
}

bool CSloganDraw::MeasureMeltStroke()
{
	CKD2DPointF	ptDPI;
	m_pD2DDeviceContext->GetDpi(&ptDPI.x, &ptDPI.y);
	CMeltProbe	probe(m_pD2DFactory, m_pDWriteFactory, m_pStrokeStyle);
	if (!probe.Create(m_sSlogan, m_sFontName, m_fFontSize, m_nFontWeight, ptDPI, m_fMeltMaxStroke))
		return false;
#if 0	// non-zero to compare text bitmap with screen text
	CKD2DRectF	rText;
	CKD2DSizeF	szScrRT = GetTextBounds(rText);
	CKD2DSizeF	szText(rText.Size());
	CComPtr<ID2D1Bitmap> pTempBmp;
	CHECK(m_pD2DDeviceContext->CreateBitmapFromWicBitmap(probe.GetBitmap(), NULL, &pTempBmp));
	CSize	szBmp(probe.GetBitmapSize());
	CKD2DRectF	rOutBmp(
		CKD2DPointF((szScrRT.width - float(szBmp.cx)) / 2, (szScrRT.height - float(szBmp.cy)) / 2), 
		CKD2DSizeF(float(szBmp.cx), float(szBmp.cy)));
	m_pD2DDeviceContext->DrawBitmap(pTempBmp, rOutBmp);
	// draw reference text
	m_pVarBrush->SetColor(D2D1::ColorF(1, 0, 0));
	m_pD2DDeviceContext->DrawTextLayout(CKD2DPointF(0, 0), m_pTextLayout, m_pVarBrush);
#endif
	return true;
}

bool CSloganDraw::TransElevator()
{
	m_pGlyphRunCB = &CSloganDraw::TransElevator;
	CHECK(m_pTextLayout->Draw(0, this, 0, 0));
	return true;
}

bool CSloganDraw::TransElevator(CKD2DPointF ptBaselineOrigin, DWRITE_MEASURING_MODE measuringMode, 
	DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, DWRITE_GLYPH_RUN const* pGlyphRun)
{
	DWRITE_GLYPH_RUN	run = *pGlyphRun;	// copy run struct to local var
	run.glyphCount = 1;	// override run's glyph count; one glyph at a time
	double	fPhase = GetPhase(GP_INVERT);
	CGlyphIter	iterGlyph(ptBaselineOrigin, pGlyphRun, true);	// tight vertical bounds
	CKD2DPointF	ptOrigin(ptBaselineOrigin);
	CKD2DRectF	rGlyph;
	UINT	iGlyph;
	while (iterGlyph.GetNext(iGlyph, rGlyph)) {	// for each glyph
		// for space testing, don't assume one-to-one mapping of glyphs to characters,
		// as that breaks in RTL languages; safer to test for a zero width bounding box
		if (rGlyph.Width() > m_fSpaceWidth) {	// exclude spaces
			rGlyph.InflateRect(AA_MARGIN, AA_MARGIN);	// add antialiasing margin
			run.glyphIndices = pGlyphRun->glyphIndices + iGlyph;
			run.glyphAdvances = iterGlyph.GetAdvancePtr();
			run.glyphOffsets = pGlyphRun->glyphOffsets + iGlyph;
			double	fGlyphWidth = rGlyph.Width();
			double	fDoorWidth = fGlyphWidth * fPhase;
			double	fDoorLeft = rGlyph.left + (fGlyphWidth - fDoorWidth) / 2;
			double	fDoorRight = fDoorLeft + fDoorWidth;
			CKD2DRectF	rDoor(DTF(fDoorLeft), rGlyph.top, DTF(fDoorRight), rGlyph.bottom);
			m_pD2DDeviceContext->PushAxisAlignedClip(rDoor, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
			m_pD2DDeviceContext->DrawGlyphRun(ptOrigin, &run, m_pDrawBrush, measuringMode);
			m_pD2DDeviceContext->PopAxisAlignedClip();
		}
		ptOrigin = iterGlyph.GetOrigin();	// update glyph origin
	}
	return true;
}

bool CSloganDraw::TransClock()
{
	m_pPathGeom.Release();
	CHECK(m_pD2DFactory->CreatePathGeometry(&m_pPathGeom));
	CComPtr<ID2D1GeometrySink> pGeomSink;
	CHECK(m_pPathGeom->Open(&pGeomSink));
	double	fPhase = GetPhase(GP_INVERT);
	AddPieWedge(pGeomSink, CKD2DPointF(0, 0), CKD2DSizeF(0.5, 0.5), 180, DTF(fPhase));
	CHECK(pGeomSink->Close());
	m_pGlyphRunCB = &CSloganDraw::TransClock;
	CHECK(m_pTextLayout->Draw(0, this, 0, 0));
	return true;
}

bool CSloganDraw::TransClock(CKD2DPointF ptBaselineOrigin, DWRITE_MEASURING_MODE measuringMode, 
	DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, DWRITE_GLYPH_RUN const* pGlyphRun)
{
	DWRITE_GLYPH_RUN	run = *pGlyphRun;	// copy run struct to local var
	run.glyphCount = 1;	// override run's glyph count; one glyph at a time
	// use font metrics for vertical bounds, so pie centers are vertically aligned
	CGlyphIter	iterGlyph(ptBaselineOrigin, pGlyphRun);
	CKD2DPointF	ptOrigin(ptBaselineOrigin);
	CKD2DRectF	rGlyph;
	UINT	iGlyph;
	while (iterGlyph.GetNext(iGlyph, rGlyph)) {	// for each glyph
		if (rGlyph.Width() > m_fSpaceWidth) {	// exclude spaces
			rGlyph.InflateRect(AA_MARGIN, AA_MARGIN);	// add antialiasing margin
			run.glyphIndices = pGlyphRun->glyphIndices + iGlyph;
			run.glyphAdvances = iterGlyph.GetAdvancePtr();
			run.glyphOffsets = pGlyphRun->glyphOffsets + iGlyph;
			CKD2DPointF	ptCtr(rGlyph.CenterPoint());
			double	fSize = max(rGlyph.Width(), rGlyph.Height()) * M_SQRT2;
			CKD2DSizeF	szScale(DTF(-fSize), DTF(fSize));	// circle enclosing glyph
			m_pD2DDeviceContext->PushLayer(D2D1::LayerParameters1(
				rGlyph, m_pPathGeom, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
				D2D1::Matrix3x2F::Scale(szScale, CKD2DPointF(0, 0))
				* D2D1::Matrix3x2F::Translation(ptCtr.x, ptCtr.y)), m_pLayer);
			m_pD2DDeviceContext->DrawGlyphRun(ptOrigin, &run, m_pDrawBrush, measuringMode);
			m_pD2DDeviceContext->PopLayer();
		}
		ptOrigin = iterGlyph.GetOrigin();	// update glyph origin
	}
	return true;
}

bool CSloganDraw::TransSkew()
{
	m_pGlyphRunCB = &CSloganDraw::TransSkew;
	CHECK(m_pTextLayout->Draw(0, this, 0, 0));
	return true;
}

bool CSloganDraw::TransSkew(CKD2DPointF ptBaselineOrigin, DWRITE_MEASURING_MODE measuringMode, 
	DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, DWRITE_GLYPH_RUN const* pGlyphRun)
{
	double	fPhase = !IsTransOut() ? (1 - m_fTransProgress) : -m_fTransProgress;
	auto mSkew(D2D1::Matrix3x2F::Skew(DTF(90 * fPhase), 0, ptBaselineOrigin));
	m_pD2DDeviceContext->SetTransform(mSkew);
	m_pD2DDeviceContext->DrawGlyphRun(ptBaselineOrigin, pGlyphRun, m_pDrawBrush, measuringMode);
	ResetTransform(m_pD2DDeviceContext);	// remove transform
	return true;
}

bool CSloganDraw::TransExplode()
{
	if (m_bIsTransStart) {	// if start of transition
		m_triSink.OnStartTrans(m_pD2DDeviceContext->GetSize());
	}
	m_triSink.OnDraw();
	m_pGlyphRunCB = &CSloganDraw::TransExplode;
	CHECK(m_pTextLayout->Draw(0, this, 0, 0));
	return true;
}

bool CSloganDraw::TransExplode(CKD2DPointF ptBaselineOrigin, DWRITE_MEASURING_MODE measuringMode, 
	DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, DWRITE_GLYPH_RUN const* pGlyphRun)
{
	const DWRITE_GLYPH_RUN& run = *pGlyphRun;
	CGlyphIter	iterGlyph(ptBaselineOrigin, pGlyphRun, true);	// tight vertical bounds
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
				run.glyphIndices + iGlyph, iterGlyph.GetAdvancePtr(), 
				run.glyphOffsets + iGlyph, 1, run.isSideways, run.bidiLevel & 1, pGeomSink));
			CHECK(pGeomSink->Close());	// close geometry sink
			CKD2DPointF	ptLayoutOrigin(fOriginX, ptBaselineOrigin.y);	// layout left edge and baseline
			CHECK(m_triSink.TessellateGlyph(ptLayoutOrigin, rGlyph, pPathGeom));	// tessellate glyph
		}
		iterGlyph.Reset();	// reset glyph iterator, as we reuse it below
	}
	CD2DSaveTransform	transSave(m_pD2DDeviceContext);	// save transform, restore on exit
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
	}
	return true;
}

bool CSloganDraw::TransSubmarine()
{
	m_pGlyphRunCB = &CSloganDraw::TransSubmarine;
	CHECK(m_pTextLayout->Draw(0, this, 0, 0));	// call text renderer
	return true;
}

bool CSloganDraw::TransSubmarine(CKD2DPointF ptBaselineOrigin, DWRITE_MEASURING_MODE measuringMode, 
	DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, DWRITE_GLYPH_RUN const* pGlyphRun)
{
	double	fPhase = GetPhase(GP_EASING | GP_EASE_BOTH);	// symmetrical easing
	CKD2DRectF	rRun;
	GetRunBounds(rRun, ptBaselineOrigin, pGlyphRun, true);	// tight vertical bounds
	rRun.InflateRect(AA_MARGIN, AA_MARGIN);
	ptBaselineOrigin.y += DTF(rRun.Height() * fPhase);
	m_pD2DDeviceContext->PushAxisAlignedClip(rRun, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
	m_pD2DDeviceContext->DrawGlyphRun(ptBaselineOrigin, pGlyphRun, m_pDrawBrush, measuringMode);
	m_pD2DDeviceContext->PopAxisAlignedClip();
	return true;
}

bool CSloganDraw::TransTumble()
{
	m_bIsGlyphRising = false;
	m_pGlyphRunCB = &CSloganDraw::TransTumble;
	CHECK(m_pTextLayout->Draw(0, this, 0, 0));	// call text renderer
	return true;
}

bool CSloganDraw::TransTumble(CKD2DPointF ptBaselineOrigin, DWRITE_MEASURING_MODE measuringMode, 
	DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, DWRITE_GLYPH_RUN const* pGlyphRun)
{
	CGlyphIter	iterGlyph(ptBaselineOrigin, pGlyphRun, true);	// tight vertical bounds
	CKD2DRectF	rGlyph;
	UINT	iGlyph;
	if (!iterGlyph.GetNext(iGlyph, rGlyph))	// get first glyph's rectangle
		return true;	// no glyphs, nothing to do
	float	fFirstCtrX = rGlyph.CenterPoint().x;	// first glyph's center X
	if (pGlyphRun->glyphCount > 1) {	// if multiple glyphs
		iterGlyph.SetPos(pGlyphRun->glyphCount - 1);	// move to last glyph
		iterGlyph.GetNext(iGlyph, rGlyph);	// get last glyph's rectangle
	}
	float	fLastCtrX = rGlyph.CenterPoint().x;	// last glyph's center X
	float	fRunWidth = fLastCtrX - fFirstCtrX;	// center to center in X
	iterGlyph.Reset();	// reset glyph iterator
	DWRITE_GLYPH_RUN	run = *pGlyphRun;	// copy run struct to local var
	run.glyphCount = 1;	// override run's glyph count; one glyph at a time
	double	fPhase = GetPhase(GP_INVERT | GP_EASING);	// inverted phase with easing
	float	fRotoAngle = DTF(fPhase * 360);	// rotation angle
	CKD2DSizeF	szScale(DTF(fPhase), DTF(fPhase));	// isotropic scaling
	CKD2DSizeF	szMaxGlyph = iterGlyph.CalcMaxGlyphBounds();	// compute max glyph bounds
	iterGlyph.Reset();	// reset glyph iterator
	CD2DSaveTransform	transSave(m_pD2DDeviceContext);	// save transform, restore on exit
	CKD2DPointF	ptOrigin(ptBaselineOrigin);
	while (iterGlyph.GetNext(iGlyph, rGlyph)) {	// for each glyph
		if (rGlyph.Width() > m_fSpaceWidth) {	// exclude spaces
			CKD2DPointF	ptCenter(rGlyph.CenterPoint());
			// glyph center's horizontal position within run, normalized to [-1, 1]
			// so that first glyph's center X == -1 and last glyph's center X == 1
			double	fNormCtrX;
			if (fRunWidth) {	// if non-zero run width
				// normalized delta X divided by run width, centered and rescaled
				fNormCtrX = ((ptCenter.x - fFirstCtrX) / fRunWidth - 0.5) * 2;
			} else {	// single glyph; avoid divide by zero
				fNormCtrX = 0;	// center of run
			}
			double fNormCtrY = m_bIsGlyphRising ? -1 : 1;	// alternate rise and fall
			m_bIsGlyphRising ^= 1;	// update alternation state
			CKD2DSizeF	szTrans(
				DTF((1 - fPhase) * fNormCtrX * szMaxGlyph.width),
				DTF((1 - fPhase) * fNormCtrY * szMaxGlyph.height));
			run.glyphIndices = pGlyphRun->glyphIndices + iGlyph;
			run.glyphAdvances = iterGlyph.GetAdvancePtr();
			run.glyphOffsets = pGlyphRun->glyphOffsets + iGlyph;
			m_pD2DDeviceContext->SetTransform(
				D2D1::Matrix3x2F::Scale(szScale, ptCenter)	// scale
				* D2D1::Matrix3x2F::Rotation(fRotoAngle, ptCenter)	// rotation
				* D2D1::Matrix3x2F::Translation(szTrans));	// translation
			m_pD2DDeviceContext->DrawGlyphRun(ptOrigin, &run, m_pDrawBrush, measuringMode);
		}
		ptOrigin = iterGlyph.GetOrigin();	// update glyph origin
	}
	return true;
}

bool CSloganDraw::TransIris()
{
	if (m_bIsTransStart) {	// if start of transition
		m_pPathGeom.Release();
		CHECK(m_pD2DFactory->CreatePathGeometry(&m_pPathGeom));	// create path geometry
		CComPtr<ID2D1GeometrySink> pGeomSink;
		CHECK(m_pPathGeom->Open(&pGeomSink));
		AddEllipse(pGeomSink, CKD2DPointF(0, 0), CKD2DSizeF(0.5, 0.5));	// unit ellipse
		CHECK(pGeomSink->Close());
		m_pLayer.Release();
		CHECK(m_pD2DDeviceContext->CreateLayer(NULL, &m_pLayer));	// create layer
	}
	m_pGlyphRunCB = &CSloganDraw::TransIris;
	CHECK(m_pTextLayout->Draw(0, this, 0, 0));	// call text renderer
	return true;
}

bool CSloganDraw::TransIris(CKD2DPointF ptBaselineOrigin, DWRITE_MEASURING_MODE measuringMode, 
	DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, DWRITE_GLYPH_RUN const* pGlyphRun)
{
	DWRITE_GLYPH_RUN	run = *pGlyphRun;	// copy run struct to local var
	run.glyphCount = 1;	// override run's glyph count; one glyph at a time
	double	fPhase = GetPhase(GP_INVERT) * M_SQRT2;	// ensure ellipse covers bounding rect
	CGlyphIter	iterGlyph(ptBaselineOrigin, pGlyphRun, true);	// tight vertical bounds
	CKD2DPointF	ptOrigin(ptBaselineOrigin);
	CKD2DRectF	rGlyph;
	UINT	iGlyph;
	while (iterGlyph.GetNext(iGlyph, rGlyph)) {	// for each glyph
		if (rGlyph.Width() > m_fSpaceWidth) {	// exclude spaces
			rGlyph.InflateRect(AA_MARGIN, AA_MARGIN);	// add antialiasing margin
			run.glyphIndices = pGlyphRun->glyphIndices + iGlyph;
			run.glyphAdvances = iterGlyph.GetAdvancePtr();
			run.glyphOffsets = pGlyphRun->glyphOffsets + iGlyph;
			CKD2DPointF	ptCtr(rGlyph.CenterPoint());
			CKD2DSizeF	szScale(DTF(rGlyph.Width() * fPhase), DTF(rGlyph.Height() * fPhase));
			m_pD2DDeviceContext->PushLayer(D2D1::LayerParameters1(
				rGlyph, m_pPathGeom, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
				D2D1::Matrix3x2F::Scale(szScale, CKD2DPointF(0, 0))
				* D2D1::Matrix3x2F::Translation(ptCtr.x, ptCtr.y)), m_pLayer);
			m_pD2DDeviceContext->DrawGlyphRun(ptOrigin, &run, m_pDrawBrush, measuringMode);
			m_pD2DDeviceContext->PopLayer();
		}
		ptOrigin = iterGlyph.GetOrigin();	// update glyph origin
	}
	return true;
}

double CSloganDraw::PowerCurve(double fPhase, double fPower)
{
	return (pow(fPower, fPhase) - 1) / (fPower - 1);
}

bool CSloganDraw::TransBlur()
{
	CKD2DRectF	rRT(CKD2DPointF(0, 0), m_pD2DDeviceContext->GetSize());
	if (m_pEffect == NULL) {	// if effect doesn't exist
		CHECK(CreateCompatibleBitmap(m_pD2DDeviceContext, &m_pOffTarget));
		CHECK(m_pD2DDeviceContext->CreateEffect(CLSID_D2D1GaussianBlur, &m_pEffect));
		m_pEffect->SetInput(0, m_pOffTarget);	// connect blur input to offscreen target
		m_pEffect->GetOutput(&m_pEffectOutImage);	// connect blur output to effect out image
		CHECK(m_pD2DDeviceContext->CreateImageBrush(m_pEffectOutImage, 
			D2D1::ImageBrushProperties(rRT), D2D1::BrushProperties(0.0f), &m_pImageBrush));
	}
	const double fPower = 33;	// favor legible portion of effect
	double	fPhase = PowerCurve(GetPhase(), fPower);
	float fMaxBlurRadius = m_fFontSize;
	CComPtr<ID2D1Image>	pTarget;
	m_pD2DDeviceContext->GetTarget(&pTarget);	// save screen target
	m_pD2DDeviceContext->SetTarget(m_pOffTarget);	// set offscreen target
	m_pD2DDeviceContext->Clear(m_clrBkgnd);	// clear offscreen target to background color
	m_pD2DDeviceContext->DrawTextLayout(CKD2DPointF(0, 0), m_pTextLayout, m_pDrawBrush);	// draw text
	m_pD2DDeviceContext->SetTarget(pTarget);	// restore screen target
	m_pEffect->SetValue(D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION, DTF(fPhase * fMaxBlurRadius));
	m_pImageBrush->SetOpacity(DTF(1 - fPhase));	// reduce opacity as blur increases
	m_pD2DDeviceContext->FillRectangle(rRT, m_pImageBrush);	// fill with image of text
	return true;
}
