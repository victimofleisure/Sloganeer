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

*/

#include "stdafx.h"
#include "SloganDraw.h"
#include <algorithm>	// for swap

#define SEED_WITH_TIME 1	// non-zero to seed pseudorandom sequence with current time

#define DTF(x) static_cast<float>(x)

#define CHECK(x) { HRESULT hr = x; if (FAILED(hr)) { OnError(hr, __FILE__, __LINE__, __DATE__); return false; }}

CSloganDraw::CSloganDraw() :
	m_clrBkgnd(D2D1::ColorF::Black),
	m_clrDraw(D2D1::ColorF::White)
{
	m_nWakeTime = 0;
	m_fTransProgress = 0;
	m_bThreadExit = false;
	m_bIsFullScreen = false;
	m_bIsTransStart = false;
	m_bSeqSlogans = false;
	m_iState = ST_TRANS_OUT;
	m_iSlogan = 0;
	m_iTransType = 0;
	m_nHoldDuration = 1000;
	m_fTransDuration = 2.0f;
	m_fFontSize = 150.0f;
	m_sFontName = L"Helvetica";
	m_nFontWeight = DWRITE_FONT_WEIGHT_BLACK;
	m_fTileSize = 0;
	m_szTileLayout = CSize(0, 0);
	m_ptTileOffset = CD2DPointF(0, 0);
}

CSloganDraw::~CSloganDraw()
{
	Destroy();
}

bool CSloganDraw::Create(HWND hWnd)
{
	if (!m_evtWake.Create(NULL, false, false, NULL))	// create wake event
		return false;
	m_timerTrans.Reset();	// reset transition timer
	m_rlTransType.Init(TRANS_TYPES);	// init transition type random list
	return CreateThread(hWnd);	// create render thread
}

void CSloganDraw::Destroy()
{
	m_bThreadExit = true;	// request render thread to exit
	m_evtWake.Set();	// set wake event to signaled
	DestroyThread();	// destroy render thread
#if CAPTURE_FRAMES
	m_capture.Destroy();	// destroy capture instance
#endif
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

void CSloganDraw::SetSlogans(const CStringArrayEx& aSlogan)
{
	ASSERT(m_pThread == NULL);	// only safe before thread is launched
	m_aSlogan = aSlogan;
	m_rlSloganIdx.Init(aSlogan.GetSize());	// init slogan random list
}

void CSloganDraw::SetSlogans(const LPCTSTR *aSlogan, int nSlogans)
{
	ASSERT(m_pThread == NULL);	// only safe before thread is launched
	m_aSlogan.SetSize(nSlogans);	// allocate slogan array
	for (int iSlogan = 0; iSlogan < nSlogans; iSlogan++) {	// for each slogan
		m_aSlogan[iSlogan] = aSlogan[iSlogan];	// copy string to array element
	}
	m_rlSloganIdx.Init(nSlogans);	// init slogan random list
}

void CSloganDraw::OnError(HRESULT hr, LPCSTR pszSrcFileName, int nLineNum, LPCSTR pszSrcFileDate)
{
	printf("%x %s %d %s\n", hr, pszSrcFileName, nLineNum, pszSrcFileDate);
}

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
	StartCycle();	// start the first cycle
	return true;
}

void CSloganDraw::OnResize()
{
	OnTextChange();
}

void CSloganDraw::StartTrans(int nState)
{
	ASSERT(nState == ST_TRANS_IN || nState == ST_TRANS_OUT);	// else logic error
	m_iState = nState;	// set state
	m_bIsTransStart = true;	// set transition start flag
	m_timerTrans.Reset();	// reset transition timer
	m_iTransType = m_rlTransType.GetNext(m_iTransType);	// get next transition type
	m_pD2DDeviceContext->SetTransform(D2D1::Matrix3x2F::Identity());	// remove transform if any
}

void CSloganDraw::StartCycle()
{
	StartTrans(ST_TRANS_IN);
	if (m_bSeqSlogans) {	// if showing slogans sequentially
		m_iSlogan++;	// next slogan index
		if (m_iSlogan >= m_aSlogan.GetSize())	// if reached end of slogans
			m_iSlogan = 0;	// reset to first slogan
	} else {	// randomizing slogans
		m_iSlogan = m_rlSloganIdx.GetNext(m_iSlogan);	// get next slogan index
	}
	OnTextChange();	// update text
}

void CSloganDraw::StartHold()
{
	m_nWakeTime = GetTickCount64() + m_nHoldDuration;	// set wake time
	m_iState = ST_HOLD;	// set state to hold
}

void CSloganDraw::ContinueHold()
{
	ULONGLONG	nNow = GetTickCount64();
	if (nNow < m_nWakeTime) {	// if more hold time remains
#if CAPTURE_FRAMES
		return;	// draw during hold state, so hold state gets captured
#else
		// sleep during hold state to reduce power usage
		DWORD	nTimeout = static_cast<DWORD>(m_nWakeTime - nNow);
		// wait for wake signal or timeout
		DWORD	nRet = WaitForSingleObject(m_evtWake, nTimeout);
		if (nRet == WAIT_OBJECT_0)	// if wake signal
			return;	// incomplete hold; return without altering state
#endif
	}
	// hold state completed
	StartTrans(ST_TRANS_OUT);	// start transition out
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
		L"",	//locale
		&m_pTextFormat	// receives text format instance
	));
	m_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	m_pTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
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

CKD2DRectF CSloganDraw::GetTextBounds() const
{
	// layout box is entire render target
	CD2DSizeF	szRT(m_pD2DDeviceContext->GetSize());
	// use text metrics for x-axis, overhang metrics for y-axis;
	// assuming text fits within the frame, overhang metrics are
	// NEGATIVE distances in DIPs from frame edge to text edge
	CKD2DRectF	rText(
		m_textMetrics.left,
		-m_overhangMetrics.top, 
		m_textMetrics.left + m_textMetrics.width, 
		szRT.height + m_overhangMetrics.bottom
	);
	rText.InflateRect(AA_MARGIN, AA_MARGIN);	// add antialiasing margin
	return rText;
}

void CSloganDraw::TransScroll()
{
	CD2DSizeF	szRT(m_pD2DDeviceContext->GetSize());
	double	fPhase;
	if (m_iState == ST_TRANS_OUT) {	// if transition out
		fPhase = m_fTransProgress; 
	} else {	// hold or transition in
		fPhase = m_fTransProgress - 1;	// reverse progress
	}
	if (m_iTransType == TT_SCROLL_RL || m_iTransType == TT_SCROLL_BT)	// if reversed
		fPhase = -fPhase;	// negate phase
	CD2DPointF	ptOrigin(0, 0);
	switch (m_iTransType) {
	case TT_SCROLL_LR:
	case TT_SCROLL_RL:
		ptOrigin.x = DTF((szRT.width - m_textMetrics.left + AA_MARGIN) * fPhase);
		break;
	case TT_SCROLL_TB:
	case TT_SCROLL_BT:
		ptOrigin.y = DTF((szRT.height + m_overhangMetrics.top + AA_MARGIN) * fPhase);
		break;
	default:
		NODEFAULTCASE;	// improper call
	}
	m_pD2DDeviceContext->DrawTextLayout(ptOrigin, m_pTextLayout, m_pDrawBrush); // draw text
}

void CSloganDraw::TransReveal()
{
	CKD2DRectF	rText(GetTextBounds());
	CD2DSizeF	szText(rText.Size());
	switch (m_iTransType) {
	case TT_REVEAL_LR:
		{
			float	fOffset = DTF(szText.width * m_fTransProgress);
			if (m_iState == ST_TRANS_OUT)	// if transition out
				fOffset -= szText.width;	// start unmasked
			rText.OffsetRect(fOffset, 0);	// offset text mask horizontally
		}
		break;
	case TT_REVEAL_TB:
		{
			float	fOffset = DTF(szText.height * m_fTransProgress);
			if (m_iState == ST_TRANS_OUT)	// if transition out
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

void CSloganDraw::TransFade()
{
	double	fBright = DTF(m_fTransProgress);
	if (m_iState == ST_TRANS_OUT)	// if transition out
		fBright = 1 - fBright;	// invert brightness
	m_pVarBrush->SetColor(D2D1::ColorF(DTF(fBright), DTF(fBright), DTF(fBright)));
	CD2DPointF	ptOrigin(0, 0);
	m_pD2DDeviceContext->DrawTextLayout(ptOrigin, m_pTextLayout, m_pVarBrush); // draw text
}

void CSloganDraw::TransTypewriter()
{
	int	nTextLen = m_aSlogan[m_iSlogan].GetLength();
	int	nCharsTyped = Round(nTextLen * m_fTransProgress);
	DWRITE_TEXT_RANGE	trShow = {0, nCharsTyped};
	DWRITE_TEXT_RANGE	trHide = {nCharsTyped, nTextLen - nCharsTyped};
	if (m_iState == ST_TRANS_OUT) {	// if transition out
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
	if (m_iState == ST_TRANS_OUT) {	// if transition out
		fScale = DTF(1 - m_fTransProgress);
	} else {	// hold or transition in
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
	CD2DSizeF	szRT(m_pD2DDeviceContext->GetSize());
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
	CRandList	rlTile(nTiles);	// initialize random list
	for (int iTile = 0; iTile < nTiles; iTile++) {	// for each tile
		m_aTileIdx[iTile] = rlTile.GetNext();	// set array element to random tile index
	}
}

void CSloganDraw::TransTile()
{
	CKD2DRectF	rText(GetTextBounds());
	if (m_bIsTransStart) {	// if start of transition
		InitTiling(rText);	// initialize tiling; only once per transition
	}
	CD2DPointF	ptOrigin(0, 0);
	m_pD2DDeviceContext->DrawTextLayout(ptOrigin, m_pTextLayout, m_pDrawBrush);	// draw text
	float	fPhase;
	if (m_iState == ST_TRANS_OUT) {	// if transition out
		fPhase = DTF(m_fTransProgress);
	} else {	// hold or transition in
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

bool CSloganDraw::OnDraw()
{
	m_pD2DDeviceContext->Clear(m_clrBkgnd);	// clear frame to background color
	double	fTransElapsed = m_timerTrans.Elapsed();	// elapsed time since transition started
	double	fTransRemain = m_fTransDuration - fTransElapsed;	// remaining transition time
	if (fTransRemain > 0) {	// if transition has time remaining
		m_fTransProgress = 1 - double(fTransRemain) / m_fTransDuration;
	} else {	// transition is complete
		m_fTransProgress = 1;
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
	default:
		NODEFAULTCASE;	// logic error
	}
	m_bIsTransStart = false;	// reset transition start flag
	switch (m_iState) {
	case ST_TRANS_IN:
		if (m_fTransProgress >= 1) {	// if transition in completed
			StartHold();	// start hold state
		}
		break;
	case ST_HOLD:
		if (!m_bThreadExit) {	// if exit wasn't requested
			ContinueHold();	// continue hold state
		}
		break;
	case ST_TRANS_OUT:
		if (m_fTransProgress >= 1) {	// if transition out completed
			StartCycle();	// start a new cycle
		}
		break;
	default:
		NODEFAULTCASE;	// logic error
	}
#if CAPTURE_FRAMES
	if (!m_capture.IsCreated()) {
		// optionally specify a destination folder for the image sequence; default is current directory
		LPCTSTR	pszOutFolderPath = NULL;
		if (!m_capture.Create(m_pD3DDevice, m_pSwapChain, pszOutFolderPath))	// create capture instance
			return false;
	}
	m_capture.CaptureFrame();
#endif
	return true;
}
