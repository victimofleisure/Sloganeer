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
		07		07nov25	add melt transition
		08		09nov25	fix typewriter breaking fade; reset drawing effect
		09		10nov25	add regression test
		10		11nov25	add elevator and clock transitions

*/

#include "stdafx.h"
#include "Sloganeer.h"
#include "SloganDraw.h"
#include <algorithm>	// for swap
#define _USE_MATH_DEFINES
#include "math.h"

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
	m_fMeltMaxStroke = 0;
	m_nFrames = 0;
	m_nSwapChainBuffers = 0;
#if SD_CAPTURE	// if capturing frames
	m_capture.m_pParent = this;
#endif	// SD_CAPTURE
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
	// order matters
	m_thrMeltWorker.Destroy();	// destroy melt worker thread
	m_bThreadExit = true;	// request render thread to exit
	m_evtWake.Set();	// set wake event to signaled
	DestroyThread();	// destroy render thread
#if SD_CAPTURE	// if capturing frames
	m_capture.Destroy();	// destroy capture instance
#endif	// SD_CAPTURE
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
	theApp.OnError(hr, pszSrcFileName, nLineNum, pszSrcFileDate);
}

#if SD_CAPTURE	// if capturing frames
void CSloganDraw::CMyD2DCapture::OnError(HRESULT hr, LPCSTR pszSrcFileName, int nLineNum, LPCSTR pszSrcFileDate)
{
	m_pParent->OnError(hr, pszSrcFileName, nLineNum, pszSrcFileDate);
}
#endif	// SD_CAPTURE

bool CSloganDraw::CreateUserResources()
{
	CHECK(m_pD2DDeviceContext->CreateSolidColorBrush(m_clrBkgnd, &m_pBkgndBrush));
	CHECK(m_pD2DDeviceContext->CreateSolidColorBrush(m_clrDraw, &m_pDrawBrush));
	CHECK(m_pD2DDeviceContext->CreateSolidColorBrush(m_clrDraw, &m_pVarBrush));
	CHECK(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(m_pDWriteFactory),
		reinterpret_cast<IUnknown **>(&m_pDWriteFactory)));
	CreateStrokeStyle();
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
	m_pStrokeStyle.Release();
	if (m_bThreadExit)
		CoUninitialize();	// needed for WIC
}

bool CSloganDraw::OnThreadCreate()
{
	CHECK(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED));	// needed for WIC
	if (SEED_WITH_TIME) {
		srand(GetTickCount());	// seed pseudorandom generator with time
	}
	if (m_bSeqSlogans) {	// if showing slogans sequentially
		m_iSlogan = -1;	// avoid skipping first slogan
	}
	LaunchMeltWorker();
	StartSlogan();	// start the first slogan
#if SD_CAPTURE	// if capturing frames
	if (!SetCapture())
		return false;
#endif	// SD_CAPTURE
	return true;
}

bool CSloganDraw::CreateStrokeStyle()
{
	CHECK(CMeltProbeWorker::CreateStrokeStyle(m_pD2DFactory, &m_pStrokeStyle));
	return true;
}

bool CSloganDraw::ResetDrawingEffect()
{
	ASSERT(m_pTextLayout != NULL);
	UINT	nTextLen = static_cast<UINT>(m_sSlogan.GetLength());
	DWRITE_TEXT_RANGE	tr = {0, nTextLen};	// for all characters in layout
	CHECK(m_pTextLayout->SetDrawingEffect(NULL, tr));	// remove drawing effect
	return true;
}

void CSloganDraw::OnResize()
{
	OnTextChange();
}

#if SD_CAPTURE	// if capturing frames
bool CSloganDraw::SetCapture(bool bEnable)
{
	if (bEnable == m_capture.IsCreated())	// if state unchanged
		return true;	// nothing to do
	if (bEnable) {	// if starting capture
		DXGI_SWAP_CHAIN_DESC	desc;
		CHECK(m_pSwapChain->GetDesc(&desc));
		m_nSwapChainBuffers = desc.BufferCount;	// store swap chain buffer count
		LPCTSTR	pszCaptureFolderPath = NULL;	// default is current directory
		if (!m_capture.Create(m_pD3DDevice, m_pSwapChain, pszCaptureFolderPath))	// create capture instance
			return false;
	} else {	// ending capture
		m_capture.Destroy();
	}
	return true;
}

bool CSloganDraw::CaptureFrame()
{
	if (!m_capture.IsCreated())	// if capture instance not created
		return false;
	//
	// The swap chain's buffers are initially blank. If the swap chain
	// has N buffers, you must present N times before the swap chain's
	// buffer zero contains meaningful data. The first N frames captured
	// are leftovers from before drawing started, and should be skipped.
	//
	if (m_nFrames < m_nSwapChainBuffers)	// if initial blank frame
		return true;	// skip blank frame; not an error
	return m_capture.CaptureFrame();	// capture frame
}
#endif	// SD_CAPTURE

#if SD_CAPTURE >= SD_CAPTURE_MAKE_REF_IMAGES	// if regression testing
bool CSloganDraw::RegressionTest()
{
	enum {
		REF_FRAMES = TRANS_TYPES * 2,	// reference frames
		TRANS_PERMS = REF_FRAMES * TRANS_TYPES,	// transition permutations
	};
	// either generate reference images, or generate images for all transition
	// permutations and check them against previously generated reference images
	const bool	bMakeRefs = SD_CAPTURE == SD_CAPTURE_MAKE_REF_IMAGES;
	const UINT	nOutputFrames = bMakeRefs ? REF_FRAMES : TRANS_PERMS;
	// swap chain buffers are initially blank; CaptureFrame supresses
	// those initial blank frames, and we must account for that here,
	// plus one extra because OnDraw calls us before CaptureFrame
	const UINT	nEndFrame = nOutputFrames + m_nSwapChainBuffers + 1;
	if (m_nFrames == nEndFrame) {	// if last frame captured
		CWnd*	pMainWnd = theApp.m_pMainWnd;
		ASSERT(pMainWnd != NULL);	// sanity check
		SetCapture(false);	// stop capturing immediately
		if (bMakeRefs) {	// if making reference images
			pMainWnd->PostMessage(WM_QUIT);	// exit app
		} else {	// checking permutations against reference
			static const LPCTSTR CAPTURE_FILENAME_FORMAT = _T("cap%06d.tif");
			CString	sRefFolder(_T("C:\\Chris\\MyProjects\\Sloganeer\\testref"));
			CIntArrayEx	aDiff;	// array of differences, as frame indices
			for (int iFrame = 0; iFrame < TRANS_PERMS; iFrame++) {	// for each frame
				bool	bIsOddFrame = iFrame & 1;
				int	iRefFrame = bIsOddFrame ? iFrame % REF_FRAMES
					: iFrame / REF_FRAMES * 2;
				CString	sFileName;
				sFileName.Format(CAPTURE_FILENAME_FORMAT, iRefFrame);
				CString	sRefPath = sRefFolder + '\\' + sFileName;
				sFileName.Format(CAPTURE_FILENAME_FORMAT, iFrame);
				CString	sTestPath(sFileName);
				TRY {
					// compare files method throws MFC exceptions
					if (!FilesEqual(sTestPath, sRefPath))	// if frames differ
						aDiff.Add(iFrame);	// add frame index to error list
				}
				CATCH (CException, e) {
					e->ReportError();
					pMainWnd->PostMessage(WM_QUIT);	// exit app
					return false;
				}
				END_CATCH
			}
			CString	sMsg(_T("Regression Test\n\n"));
			UINT	nMBFlags = MB_OK;
			int	nDiffs = aDiff.GetSize();
			if (nDiffs) {	// if differences found
				CString	sVal;
				sVal.Format(_T("FAIL: %d unexpected frames\n"), nDiffs);
				sMsg += sVal;
				const int	nMaxDiffs = 100;	// keep dialog reasonable
				for (int iDiff = 0; iDiff < min(nDiffs, nMaxDiffs); iDiff++) {
					CString	sVal;
					if (iDiff)
						sMsg += _T(", ");	// add separator
					sVal.Format(_T("%d"), aDiff[iDiff]);
					sMsg += sVal;
				}
				if (nDiffs >= nMaxDiffs)	// if too many differences
					sMsg += _T(" ...");	// indicate overflow
				nMBFlags |= MB_ICONERROR;
			} else {	// no differences
				sMsg += _T("Pass");	// all good
				nMBFlags |= MB_ICONINFORMATION;
			}
			AfxMessageBox(sMsg, nMBFlags);
			pMainWnd->PostMessage(WM_QUIT);	// exit app
		}
	}
	const bool	bIsOddFrame = m_nFrames & 1;
	const int	iState = bIsOddFrame ? ST_TRANS_OUT : ST_TRANS_IN;
	StartTrans(iState, 1);	// start a new transition
	// override transition type
	if (bMakeRefs) {	// if generating reference frames
		m_iTransType = (m_nFrames / 2) % TRANS_TYPES;
	} else {	// checking permutations against reference
		m_iTransType = bIsOddFrame ? (m_nFrames / 2) % TRANS_TYPES 
			: m_nFrames / REF_FRAMES % TRANS_TYPES;
	}
	m_fTransProgress = 1.0 / 3;	// override transition progress
	m_nHoldDuration = 0;	// no hold
	m_nPauseDuration = 0;	// no pause either
	srand(0);	// ensure consistent behavior for transitions that use randomness
	return true;
}
#endif	// SD_CAPTURE

void CSloganDraw::StartTrans(int nState, float fDuration)
{
	ASSERT(nState == ST_TRANS_IN || nState == ST_TRANS_OUT);	// else logic error
	if (nState == ST_TRANS_OUT)	{	// if starting outgoing transition
		// reset any persistent state left by incoming transition
		switch (m_iTransType) {	// previous transition type
		case TT_TYPEWRITER:
			ResetDrawingEffect();	// remove per-character brushes
			break;
		}
	}
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
	m_sSlogan = m_aSlogan[m_iSlogan];	// cache current slogan
	OnTextChange();	// update text
}

void CSloganDraw::StartIdle(int nDuration)
{
	m_nWakeTime = GetTickCount64() + nDuration;	// set wake time
}

bool CSloganDraw::ContinueIdle()
{
#if SD_CAPTURE	// if capturing frames
	return false;	// draw during idle, so idle gets captured
#endif	// SD_CAPTURE
	ULONGLONG	nNow = GetTickCount64();
	if (nNow < m_nWakeTime) {	// if more idle time remains
		// block instead of drawing, to reduce power usage
		DWORD	nTimeout = static_cast<DWORD>(m_nWakeTime - nNow);
		// wait for wake signal or timeout
		DWORD	nRet = WaitForSingleObject(m_evtWake, nTimeout);
		if (nRet == WAIT_OBJECT_0)	// if wake signal
			return false;	// idle is incomplete
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
	int	nTextLen = m_sSlogan.GetLength();
	CD2DSizeF	szRT(m_pD2DDeviceContext->GetSize());	// get render target size
	// layout box is entire frame, for full-screen text and paragraph centering
	CHECK(m_pDWriteFactory->CreateTextLayout(	// create text layout instance
		m_sSlogan,		// source text
		nTextLen,		// text length in characters
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

bool CSloganDraw::LaunchMeltWorker()
{
	CD2DPointF	ptDPI;
	m_pD2DDeviceContext->GetDpi(&ptDPI.x, &ptDPI.y);
	m_aMeltStroke.SetSize(m_aSlogan.GetSize());	// allocate destination stroke array
	return m_thrMeltWorker.Create(m_aSlogan, m_sFontName,	// launch worker thread
		m_fFontSize, m_nFontWeight, ptDPI, m_aMeltStroke);
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
	double	fPhase = m_fTransProgress;
	if (!IsTransOut())	// if incoming transition
		fPhase = 1 - fPhase;	// invert phase
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

bool CSloganDraw::TransElevator()
{
	CHECK(m_pTextLayout->Draw(0, this, 0, 0));
	return true;
}

void CSloganDraw::TransElevator(CD2DPointF ptBaselineOrigin, DWRITE_MEASURING_MODE measuringMode, 
	DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, DWRITE_GLYPH_RUN const* pGlyphRun)
{
	double	fPhase = m_fTransProgress;
	if (!IsTransOut())	// if incoming transition
		fPhase = 1 - fPhase;	// invert phase
	m_pD2DDeviceContext->DrawGlyphRun(ptBaselineOrigin, pGlyphRun, m_pDrawBrush, measuringMode);
	CGlyphIter	iterGlyph(ptBaselineOrigin, pGlyphRun);
	CKD2DRectF	rGlyph;
	UINT	iGlyph;
	while (iterGlyph.GetNext(iGlyph, rGlyph)) {	// for each glyph
		int	iChar = pGlyphRunDescription->textPosition + iGlyph;
		if (m_sSlogan[iChar] != ' ') {	// if non-blank character
			float	fGlyphWidth = pGlyphRun->glyphAdvances[iGlyph];
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
	double	fPhase = m_fTransProgress;
	if (!IsTransOut())	// if incoming transition
		fPhase = 1 - fPhase;	// invert phase
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
	iterGlyph.Reset();
	while (iterGlyph.GetNext(iGlyph, rGlyph)) {	// for each glyph
		int	iChar = pGlyphRunDescription->textPosition + iGlyph;
		if (m_sSlogan[iChar] != ' ') {	// if non-blank character
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
	case TT_MELT:
		TransMelt(ptBaselineOrigin, measuringMode, pGlyphRunDescription, &glyphRun);
		break;
	case TT_ELEVATOR:
		TransElevator(ptBaselineOrigin, measuringMode, pGlyphRunDescription, &glyphRun);
		break;
	case TT_CLOCK:
		TransClock(ptBaselineOrigin, measuringMode, pGlyphRunDescription, &glyphRun);
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
#if SD_CAPTURE >= SD_CAPTURE_MAKE_REF_IMAGES	// if regression testing
	bTransComplete = RegressionTest();
#endif	// SD_CAPTURE
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
	case TT_MELT:
		TransMelt();
		break;
	case TT_ELEVATOR:
		TransElevator();
		break;
	case TT_CLOCK:
		TransClock();
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
#if SD_CAPTURE	// if capturing frames
	CaptureFrame();
	m_nFrames++;
#endif	// SD_CAPTURE
	return true;
}
