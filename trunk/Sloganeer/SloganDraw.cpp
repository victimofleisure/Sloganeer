// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      24oct25	initial version

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
	m_iState = ST_TRANS_OUT;
	m_iSlogan = 0;
	m_iTransType = 0;
	m_nHoldDuration = 1000;
	m_fTransDuration = 2.0f;
	m_fFontSize = 150.0f;
	m_sFontName = L"Helvetica";
	m_nFontWeight = DWRITE_FONT_WEIGHT_BLACK;
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
	m_pD2DDeviceContext->CreateSolidColorBrush(m_clrBkgnd, &m_pBkgndBrush);
	m_pD2DDeviceContext->CreateSolidColorBrush(m_clrDraw, &m_pDrawBrush);
	m_pD2DDeviceContext->CreateSolidColorBrush(m_clrDraw, &m_pVarBrush);
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
		srand(GetTickCount());
	}
	StartCycle();
	return true;
}

void CSloganDraw::OnResize()
{
	OnTextChange();
}

void CSloganDraw::StartCycle()
{
	m_iState = ST_TRANS_IN;	// set state to transition in
	m_timerTrans.Reset();	// reset transition timer
	m_iSlogan = m_rlSloganIdx.GetNext(m_iSlogan);	// get next slogan index
	m_iTransType = m_rlTransType.GetNext(m_iTransType);	// get next transition type
	OnTextChange();	// update text
	m_pD2DDeviceContext->SetTransform(D2D1::Matrix3x2F::Identity());	// remove transform if any
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
		DWORD	nTimeout = static_cast<DWORD>(m_nWakeTime - nNow);
		// wait for wake signal or timeout
		DWORD	nRet = WaitForSingleObject(m_evtWake, nTimeout);
		if (nRet == WAIT_OBJECT_0)	// if wake signal
			return;	// incomplete hold; return without altering state
	}
	// hold state completed
	m_iState = ST_TRANS_OUT;	// set state to transition out
	m_timerTrans.Reset();	// reset transition timer
	m_iTransType = m_rlTransType.GetNext(m_iTransType);	// get next transition type
}

bool CSloganDraw::OnFontChange()
{
	m_pTextFormat.Release();
	CHECK(m_pDWriteFactory->CreateTextFormat(
		m_sFontName,
		NULL,	// font collection
		static_cast<DWRITE_FONT_WEIGHT>(m_nFontWeight),
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		m_fFontSize,
		L"",	//locale
		&m_pTextFormat
	));
	m_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	m_pTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
	m_pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
	return OnTextChange();
}

bool CSloganDraw::OnTextChange()
{
	m_pTextLayout.Release();
	LPCTSTR	pszSlogan = m_aSlogan[m_iSlogan];
	int	nSloganLen = m_aSlogan[m_iSlogan].GetLength();
	CD2DSizeF	szRT(m_pD2DDeviceContext->GetSize());
	CHECK(m_pDWriteFactory->CreateTextLayout(
		pszSlogan, 
		nSloganLen, 
		m_pTextFormat, 
		szRT.width, 
		szRT.height, 
		&m_pTextLayout
	));
	CHECK(m_pTextLayout->GetMetrics(&m_textMetrics));
	CHECK(m_pTextLayout->GetOverhangMetrics(&m_overhangMetrics));
	return true;
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
	m_pD2DDeviceContext->DrawTextLayout(ptOrigin, m_pTextLayout, m_pDrawBrush);
}

void CSloganDraw::TransReveal()
{
	CD2DSizeF	szRT(m_pD2DDeviceContext->GetSize());
	CD2DRectF	rMask(
		m_textMetrics.left - AA_MARGIN, 
		-m_overhangMetrics.top - AA_MARGIN, 
		m_textMetrics.left + m_textMetrics.width + AA_MARGIN, 
		szRT.height + m_overhangMetrics.bottom + AA_MARGIN
	);
	CD2DSizeF	szMask(rMask.right - rMask.left, rMask.bottom - rMask.top);
	switch (m_iTransType) {
	case TT_REVEAL_LR:
		{
			float	fOffset = DTF(szMask.width * m_fTransProgress);
			if (m_iState == ST_TRANS_OUT)	// if transition out
				fOffset -= szMask.width;	// start unmasked
			rMask.left += fOffset;
			rMask.right += fOffset;
		}
		break;
	case TT_REVEAL_TB:
		{
			float	fOffset = DTF(szMask.height * m_fTransProgress);
			if (m_iState == ST_TRANS_OUT)	// if transition out
				fOffset -= szMask.height;	// start unmasked
			rMask.top += fOffset;
			rMask.bottom += fOffset;
		}
		break;
	default:
		NODEFAULTCASE;	// improper call
	}
	CD2DPointF	ptOrigin(0, 0);
	m_pD2DDeviceContext->DrawTextLayout(ptOrigin, m_pTextLayout, m_pDrawBrush);
	m_pD2DDeviceContext->FillRectangle(rMask, m_pBkgndBrush);
}

void CSloganDraw::TransFade()
{
	double	fBright = DTF(m_fTransProgress);
	if (m_iState == ST_TRANS_OUT)	// if transition out
		fBright = 1 - fBright;	// invert brightness
	m_pVarBrush->SetColor(D2D1::ColorF(DTF(fBright), DTF(fBright), DTF(fBright)));
	CD2DPointF	ptOrigin(0, 0);
	m_pD2DDeviceContext->DrawTextLayout(ptOrigin, m_pTextLayout, m_pVarBrush);
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
	m_pTextLayout->SetDrawingEffect(m_pDrawBrush, trShow);
	m_pTextLayout->SetDrawingEffect(m_pBkgndBrush, trHide);
	CD2DPointF	ptOrigin(0, 0);
	m_pD2DDeviceContext->DrawTextLayout(ptOrigin, m_pTextLayout, m_pDrawBrush);
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
		szScale = CD2DSizeF(fScale, 1);
		break;
	case TT_SCALE_VERT:
		szScale = CD2DSizeF(1, fScale);
		break;
	case TT_SCALE_BOTH:
		szScale = CD2DSizeF(fScale, fScale);
		break;
	default:
		NODEFAULTCASE;	// improper call
	}
	CD2DSizeF	szRT(m_pD2DDeviceContext->GetSize());
	CD2DPointF	ptCenter(szRT.width / 2, szRT.height / 2);
	auto mScale(D2D1::Matrix3x2F::Scale(szScale, ptCenter));
	m_pD2DDeviceContext->SetTransform(mScale);
	CD2DPointF	ptOrigin(0, 0);
	m_pD2DDeviceContext->DrawTextLayout(ptOrigin, m_pTextLayout, m_pDrawBrush);
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
	default:
		NODEFAULTCASE;	// logic error
	}
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
	return true;
}
