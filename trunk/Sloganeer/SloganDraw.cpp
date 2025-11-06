// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      24oct25	initial version
		01		27oct25	add tile transition
		02		28oct25	add capture
		03		29oct25	add pause
		04		30oct25	move parameters to base class
		05		01nov25	add vertical converge transition
		06		06nov25	add horizontal converge transition

*/

#include "stdafx.h"
#include "Sloganeer.h"
#include "SloganDraw.h"
#include <algorithm>	// for swap

#define SEED_WITH_TIME 1	// non-zero to seed pseudorandom sequence with current time

#define DTF(x) static_cast<float>(x)

#define CHECK(x) { HRESULT hr = x; if (FAILED(hr)) { OnError(hr, __FILE__, __LINE__, __DATE__); return false; }}

CSloganDraw::CSloganDraw()
{
	Init();
}

CSloganDraw::CSloganDraw(CSloganParams& params) : CSloganParams(params)
{
	Init();
}

CSloganDraw::~CSloganDraw()
{
	Destroy();
}

void CSloganDraw::Init()
{
	m_nWakeTime = 0;
	m_fTransProgress = 0;
	m_bThreadExit = false;
	m_bIsFullScreen = false;
	m_bIsTransStart = false;
	m_iState = ST_TRANS_OUT;
	m_iSlogan = 0;
	m_iTransType = 0;
	m_fTransDuration = 2.0f;
	m_fTileSize = 0;
	m_szTileLayout = CSize(0, 0);
	m_ptTileOffset = CD2DPointF(0, 0);
	m_bIsGlyphRising = false;
	m_iGlyphLine = 0;
#if CAPTURE_FRAMES	// if capturing frames
	m_capture.m_pParent = this;
#endif	// CAPTURE_FRAMES
}

bool CSloganDraw::Create(HWND hWnd)
{
	if (!m_evtWake.Create(NULL, false, false, NULL))	// create wake event
		return false;
	m_timerTrans.Reset();	// reset transition timer
	m_rlSloganIdx.Init(m_aSlogan.GetSize());	// init slogan index shuffler
	m_rlTransType.Init(TRANS_TYPES);	// init transition type shuffler
	return CreateThread(hWnd);	// create render thread
}

void CSloganDraw::Destroy()
{
	m_bThreadExit = true;	// request render thread to exit
	m_evtWake.Set();	// set wake event to signaled
	DestroyThread();	// destroy render thread
#if CAPTURE_FRAMES	// if capturing frames
	m_capture.Destroy();	// destroy capture instance
#endif	// CAPTURE_FRAMES
}

void CSloganDraw::Resize()
{
	// don't access render thread's data in this method
	m_evtWake.Set();	// set wake event to signaled
	PushCommand(RC_RESIZE);	// enqueue command
}

bool CSloganDraw::FullScreen(bool bEnable)
{
	// don't access render thread's data in this method
	if (bEnable == m_bIsFullScreen)	// if state unchanged
		return true;	// nothing to do
	m_evtWake.Set();	// set wake event to signaled
	if (!PushCommand(CRenderCmd(RC_SET_FULLSCREEN, !m_bIsFullScreen)))	// enqueue command
		return false;
	m_bIsFullScreen ^= 1;
	return true;
}

void CSloganDraw::OnError(HRESULT hr, LPCSTR pszSrcFileName, int nLineNum, LPCSTR pszSrcFileDate)
{
#ifdef _DEBUG
	printf("%x %s %d %s\n", hr, pszSrcFileName, nLineNum, pszSrcFileDate);
#endif
	CString	sSrcFileName(pszSrcFileName);	// convert to Unicode
	CString	sSrcFileDate(pszSrcFileDate);
	CString	sErrorMsg;
	sErrorMsg.Format(_T("Error %d (0x%x) in %s line %d (%s)"), hr, hr,
		sSrcFileName.GetString(), nLineNum, sSrcFileDate.GetString());
	sErrorMsg += '\n' + FormatSystemError(hr);
	theApp.WriteLogEntry(sErrorMsg);
}

#if CAPTURE_FRAMES	// if capturing frames
void CSloganDraw::CMyD2DCapture::OnError(HRESULT hr, LPCSTR pszSrcFileName, int nLineNum, LPCSTR pszSrcFileDate)
{
	m_pParent->OnError(hr, pszSrcFileName, nLineNum, pszSrcFileDate);
}
#endif	// CAPTURE_FRAMES

bool CSloganDraw::CreateUserResources()
{
	CHECK(m_pD2DDeviceContext->CreateSolidColorBrush(m_clrBkgnd, &m_pBkgndBrush));
	CHECK(m_pD2DDeviceContext->CreateSolidColorBrush(m_clrDraw, &m_pDrawBrush));
	CHECK(m_pD2DDeviceContext->CreateSolidColorBrush(m_clrDraw, &m_pVarBrush));
	CHECK(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(m_pDWriteFactory),
		reinterpret_cast<IUnknown **>(&m_pDWriteFactory)));
	return OnFontChange();
}

void CSloganDraw::DestroyUserResources()
{
	m_pBkgndBrush.Release();
	m_pDrawBrush.Release();
	m_pVarBrush.Release();
	m_pTextFormat.Release();
	m_pTextLayout.Release();
	m_pDWriteFactory.Release();
}

bool CSloganDraw::OnThreadCreate()
{
	if (SEED_WITH_TIME) {
		srand(GetTickCount());	// seed pseudorandom generator with time
	}
	if (m_bSeqSlogans) {	// if showing slogans sequentially
		m_iSlogan = -1;	// avoid skipping first slogan
	}
	StartSlogan();	// start the first slogan
	return true;
}

void CSloganDraw::OnResize()
{
	OnTextChange();
}

void CSloganDraw::StartTrans(int nState, float fDuration)
{
	ASSERT(nState == ST_TRANS_IN || nState == ST_TRANS_OUT);	// else logic error
	m_iState = nState;	// set state
	m_fTransDuration = fDuration;
	m_bIsTransStart = true;	// set transition start flag
	m_timerTrans.Reset();	// reset transition timer
	m_iTransType = m_rlTransType.GetNext(m_iTransType);	// get next transition type
	m_pD2DDeviceContext->SetTransform(D2D1::Matrix3x2F::Identity());	// remove transform if any
}

void CSloganDraw::StartSlogan()
{
	StartTrans(ST_TRANS_IN, m_fInTransDuration);	// start incoming transition
	if (m_bSeqSlogans) {	// if showing slogans sequentially
		m_iSlogan++;	// next slogan index
		if (m_iSlogan >= m_aSlogan.GetSize())	// if reached end of slogans
			m_iSlogan = 0;	// reset to first slogan
	} else {	// randomizing slogans
		m_iSlogan = m_rlSloganIdx.GetNext(m_iSlogan);	// get next slogan index
	}
	OnTextChange();	// update text
}

void CSloganDraw::StartIdle(int nDuration)
{
	m_nWakeTime = GetTickCount64() + nDuration;	// set wake time
}

bool CSloganDraw::ContinueIdle()
{
	ULONGLONG	nNow = GetTickCount64();
	if (nNow < m_nWakeTime) {	// if more idle time remains
#if CAPTURE_FRAMES	// if capturing frames
		return false;	// draw during idle, so idle gets captured
#else	// not capturing frames
		// block instead of drawing, to reduce power usage
		DWORD	nTimeout = static_cast<DWORD>(m_nWakeTime - nNow);
		// wait for wake signal or timeout
		DWORD	nRet = WaitForSingleObject(m_evtWake, nTimeout);
		if (nRet == WAIT_OBJECT_0)	// if wake signal
			return false;	// idle is incomplete
#endif	// CAPTURE_FRAMES
	}
	return true;	// idle is over
}

bool CSloganDraw::OnFontChange()
{
	m_pTextFormat.Release();	// release previous text format instance
	CHECK(m_pDWriteFactory->CreateTextFormat(	// create text format instance
		m_sFontName,	// font name
		NULL,	// font collection
		static_cast<DWRITE_FONT_WEIGHT>(m_nFontWeight),	// font weight
		DWRITE_FONT_STYLE_NORMAL,	// font style
		DWRITE_FONT_STRETCH_NORMAL,	// font stretch
		m_fFontSize,	// font size in points
		L"",	// locale
		&m_pTextFormat	// receives text format instance
	));
	m_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	m_pTextFormat->SetWordWrapping(m_bNoWordWrap ? 
		DWRITE_WORD_WRAPPING_NO_WRAP : DWRITE_WORD_WRAPPING_WRAP);
	m_pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
	return OnTextChange();
}

bool CSloganDraw::OnTextChange()
{
	m_pTextLayout.Release();	// release previous text layout instance
	LPCTSTR	pszSlogan = m_aSlogan[m_iSlogan];
	int	nSloganLen = m_aSlogan[m_iSlogan].GetLength();
	CD2DSizeF	szRT(m_pD2DDeviceContext->GetSize());	// get render target size
	// layout box is entire frame, for full-screen text and paragraph centering
	CHECK(m_pDWriteFactory->CreateTextLayout(	// create text layout instance
		pszSlogan,		// source text
		nSloganLen,		// text length in characters
		m_pTextFormat,	// text format instance
		szRT.width,		// layout box width in DIPs
		szRT.height,	// layout box height in DIPs
		&m_pTextLayout	// receives text layout instance
	));
	CHECK(m_pTextLayout->GetMetrics(&m_textMetrics));	// get text metrics
	CHECK(m_pTextLayout->GetOverhangMetrics(&m_overhangMetrics));	// get overhang metrics
	return true;
}

CD2DSizeF CSloganDraw::GetTextBounds(CKD2DRectF& rText) const
{
	// layout box is entire render target
	CD2DSizeF	szRT(m_pD2DDeviceContext->GetSize());	// get render target size
	// use text metrics for x-axis, overhang metrics for y-axis;
	// assuming text fits within the frame, overhang metrics are
	// NEGATIVE distances in DIPs from frame edge to text edge
	rText = CKD2DRectF(
		m_textMetrics.left,
		-m_overhangMetrics.top, 
		m_textMetrics.left + m_textMetrics.width, 
		szRT.height + m_overhangMetrics.bottom
	);
	rText.InflateRect(AA_MARGIN, AA_MARGIN);	// add antialiasing margin
	return szRT;	// return render target size
}

void CSloganDraw::TransScroll()
{
	CKD2DRectF	rText;
	CD2DSizeF	szRT(GetTextBounds(rText));
	CD2DSizeF	szText(rText.Size());
	double	fPhase;
	if (IsTransOut()) {	// if outgoing transition
		fPhase = m_fTransProgress; 
	} else {	// incoming transition
		fPhase = m_fTransProgress - 1;	// reverse progress
	}
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
	double	fBright = DTF(m_fTransProgress);
	if (IsTransOut())	// if outgoing transition
		fBright = 1 - fBright;	// invert brightness
	m_pVarBrush->SetColor(D2D1::ColorF(
		DTF(Lerp(m_clrBkgnd.r, m_clrDraw.r, fBright)),
		DTF(Lerp(m_clrBkgnd.g, m_clrDraw.g, fBright)),
		DTF(Lerp(m_clrBkgnd.b, m_clrDraw.b, fBright))
	));
	CD2DPointF	ptOrigin(0, 0);
	m_pD2DDeviceContext->DrawTextLayout(ptOrigin, m_pTextLayout, m_pVarBrush); // draw text
}

void CSloganDraw::TransTypewriter()
{
	int	nTextLen = m_aSlogan[m_iSlogan].GetLength();
	int	nCharsTyped = Round(nTextLen * m_fTransProgress);
	DWRITE_TEXT_RANGE	trShow = {0, nCharsTyped};
	DWRITE_TEXT_RANGE	trHide = {nCharsTyped, nTextLen - nCharsTyped};
	if (IsTransOut()) {	// if outgoing transition
		std::swap(trShow, trHide);	// swap text ranges
	}
	m_pTextLayout->SetDrawingEffect(m_pDrawBrush, trShow);	// set brush for shown characters
	m_pTextLayout->SetDrawingEffect(m_pBkgndBrush, trHide);	// set brush for hidden characters
	CD2DPointF	ptOrigin(0, 0);
	m_pD2DDeviceContext->DrawTextLayout(ptOrigin, m_pTextLayout, m_pDrawBrush);	// draw text
}

void CSloganDraw::TransScale()
{
	float	fScale;
	if (IsTransOut()) {	// if outgoing transition
		fScale = DTF(1 - m_fTransProgress);
	} else {	// hold or incoming transition
		fScale = DTF(m_fTransProgress);
	}
	CD2DSizeF	szScale;
	switch (m_iTransType) {
	case TT_SCALE_HORZ:
		szScale = CD2DSizeF(fScale, 1);	// scale horizontally
		break;
	case TT_SCALE_VERT:
		szScale = CD2DSizeF(1, fScale);	// scale vertically
		break;
	case TT_SCALE_BOTH:
		szScale = CD2DSizeF(fScale, fScale);	// scale both axes
		break;
	default:
		NODEFAULTCASE;	// improper call
	}
	CD2DSizeF	szRT(m_pD2DDeviceContext->GetSize());	// get render target size
	CD2DPointF	ptCenter(szRT.width / 2, szRT.height / 2);	// scaling center point
	auto mScale(D2D1::Matrix3x2F::Scale(szScale, ptCenter));	// create scaling matrix
	m_pD2DDeviceContext->SetTransform(mScale);	// apply scaling matrix
	CD2DPointF	ptOrigin(0, 0);
	m_pD2DDeviceContext->DrawTextLayout(ptOrigin, m_pTextLayout, m_pDrawBrush);	// draw text
}

void CSloganDraw::InitTiling(const CKD2DRectF& rText)
{
	// ideal tile count is one tile for each frame of transition
	DWORD	dwDisplayFreq;
	GetDisplayFrequency(dwDisplayFreq);
	float	fIdealTileCount = dwDisplayFreq * m_fTransDuration;
	// compute tile size: divide text area by ideal tile count and take square root
	CD2DSizeF	szText(rText.Size());
	m_fTileSize = sqrt(szText.width * szText.height / fIdealTileCount);
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
	CKD2DRectF	rText;
	GetTextBounds(rText);
	if (m_bIsTransStart) {	// if start of transition
		InitTiling(rText);	// initialize tiling; only once per transition
	}
	CD2DPointF	ptOrigin(0, 0);
	m_pD2DDeviceContext->DrawTextLayout(ptOrigin, m_pTextLayout, m_pDrawBrush);	// draw text
	float	fPhase;
	if (IsTransOut()) {	// if outgoing transition
		fPhase = DTF(m_fTransProgress);
	} else {	// hold or incoming transition
		fPhase = DTF(1 - m_fTransProgress);
	}
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
	const CString& sSlogan = m_aSlogan[m_iSlogan];
	m_aCharToLine.SetSize(sSlogan.GetLength());	// allocate character-to-line index array
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

void CSloganDraw::TransConverge()
{
	if (!MakeCharToLineTable())
		return;
	m_bIsGlyphRising = false;
	m_iGlyphLine = 0;
	m_pTextLayout->Draw(0, this, 0, 0);
}

void CSloganDraw::TransConvergeHorz(CD2DPointF ptBaselineOrigin, DWRITE_MEASURING_MODE measuringMode, 
	DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, DWRITE_GLYPH_RUN const* pGlyphRun)
{
	// odd lines slide left and even lines slide right, or vice versa
	double	fPhase = m_fTransProgress;
	if (!IsTransOut())	// if incoming transition
		fPhase = 1 - fPhase;	// invert phase
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
	double	fPhase = m_fTransProgress;
	if (!IsTransOut())	// if incoming transition
		fPhase = 1 - fPhase;	// invert phase
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
		if (pGlyphRunDescription->string[iGlyph] != ' ') {	// exclude spaces
			if (m_bIsGlyphRising)	// if glyph rises
				fOffset = -fOffset;	// negate offset
			m_bIsGlyphRising ^= 1;	// alternate
		}
		m_aGlyphOffset[iGlyph].ascenderOffset += fOffset;
	}
	m_pD2DDeviceContext->DrawGlyphRun(ptBaselineOrigin, pGlyphRun, m_pDrawBrush, measuringMode);
}

HRESULT CSloganDraw::DrawGlyphRun(void* pClientDrawingContext, FLOAT fBaselineOriginX, 
	FLOAT fBaselineOriginY, DWRITE_MEASURING_MODE measuringMode, DWRITE_GLYPH_RUN const* pGlyphRun, 
	DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, IUnknown* pClientDrawingEffect)
{
	UINT	nGlyphs = pGlyphRun->glyphCount;
	m_aGlyphOffset.SetSize(nGlyphs);	// array is member to reduce reallocation
	if (pGlyphRun->glyphOffsets != NULL)	// if caller specified glyph offsets, copy them
		memcpy(m_aGlyphOffset.GetData(), pGlyphRun->glyphOffsets, sizeof(DWRITE_GLYPH_OFFSET) * nGlyphs);
	else {	// no glyph offsets, so zero our array
		ZeroMemory(m_aGlyphOffset.GetData(), sizeof(DWRITE_GLYPH_OFFSET) * nGlyphs);
	}
	DWRITE_GLYPH_RUN	glyphRun = *pGlyphRun;	// copy entire glyph run struct
	glyphRun.glyphOffsets = m_aGlyphOffset.GetData();	// override glyph run copy's offset array
	CD2DPointF	ptBaselineOrigin(fBaselineOriginX, fBaselineOriginY);
	switch (m_iTransType) {
	case TT_CONVERGE_HORZ:
		TransConvergeHorz(ptBaselineOrigin, measuringMode, pGlyphRunDescription, &glyphRun);
		break;
	case TT_CONVERGE_VERT:
		TransConvergeVert(ptBaselineOrigin, measuringMode, pGlyphRunDescription, &glyphRun);
		break;
	default:
		NODEFAULTCASE;
	}
	return S_OK;
}

bool CSloganDraw::OnDraw()
{
	m_pD2DDeviceContext->Clear(m_clrBkgnd);	// clear frame to background color
	double	fTransElapsed = m_timerTrans.Elapsed();	// elapsed time since transition started
	double	fTransRemain = m_fTransDuration - fTransElapsed;	// remaining transition time
	bool	bTransComplete;
	if (fTransRemain > 0) {	// if transition has time remaining
		m_fTransProgress = 1 - fTransRemain / m_fTransDuration;
		bTransComplete = false;
	} else {	// transition is complete
		m_fTransProgress = 1;
		bTransComplete = true;
	}
	switch (m_iTransType) {	// switch on transition type
	case TT_SCROLL_LR:
	case TT_SCROLL_RL:
	case TT_SCROLL_TB:
	case TT_SCROLL_BT:
		TransScroll();
		break;
	case TT_REVEAL_LR:
	case TT_REVEAL_TB:
		TransReveal();
		break;
	case TT_TYPEWRITER:
		TransTypewriter();
		break;
	case TT_FADE:
		TransFade();
		break;
	case TT_SCALE_HORZ:
	case TT_SCALE_VERT:
	case TT_SCALE_BOTH:
		TransScale();
		break;
	case TT_TILE:
		TransTile();
		break;
	case TT_CONVERGE_HORZ:
	case TT_CONVERGE_VERT:
		TransConverge();
		break;
	default:
		NODEFAULTCASE;	// logic error
	}
#if 0	// draw bounds (debug only)
	CKD2DRectF	rText;
	GetTextBounds(rText);
	m_pD2DDeviceContext->DrawRectangle(rText, m_pDrawBrush);
#endif
	m_bIsTransStart = false;	// reset transition start flag
	switch (m_iState) {
	case ST_TRANS_IN:
		if (bTransComplete) {	// if incoming transition completed
			if (m_nHoldDuration > 0) {	// if hold desired
				StartIdle(m_nHoldDuration);	// start hold idle
				m_iState = ST_HOLD;	// set state to hold
			} else {	// skip hold state
				StartTrans(ST_TRANS_OUT, m_fOutTransDuration);	// start outgoing transition
			}
		}
		break;
	case ST_HOLD:
		if (!m_bThreadExit) {	// if exit wasn't requested
			if (ContinueIdle()) {	// if hold completed
				StartTrans(ST_TRANS_OUT, m_fOutTransDuration);	// start outgoing transition
			}
		}
		break;
	case ST_TRANS_OUT:
		if (bTransComplete) {	// if outgoing transition completed
			if (m_nPauseDuration > 0) {	// if pause desired
				StartIdle(m_nPauseDuration);	// start pause idle
				m_iState = ST_PAUSE;	// set state to pause
			} else {	// skip pause state
				StartSlogan();	// start a new slogan
			}
		}
		break;
	case ST_PAUSE:
		if (!m_bThreadExit) {	// if exit wasn't requested
			if (ContinueIdle()) {	// if pause completed
				StartSlogan();	// start a new slogan
			}
		}
		break;
	default:
		NODEFAULTCASE;	// logic error
	}
#if CAPTURE_FRAMES	// if capturing frames
	if (!m_capture.IsCreated()) {
		// optionally specify a destination folder for the image sequence; default is current directory
		LPCTSTR	pszOutFolderPath = NULL;
		if (!m_capture.Create(m_pD3DDevice, m_pSwapChain, pszOutFolderPath))	// create capture instance
			return false;
	}
	m_capture.CaptureFrame();
#endif	// CAPTURE_FRAMES
	return true;
}
