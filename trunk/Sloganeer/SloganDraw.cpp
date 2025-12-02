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
		11		12nov25	add skew and spin transitions
		12		14nov25	add recording
		13		17nov25	add explode transition
		14		18nov25	add per-slogan customization
		15		22nov25	add random typewriter variant
		16		23nov25	in elevator and clock, add antialiasing margin
		17		24nov25	in explode transition, set bidi level
		18		24nov25	move transitions to separate cpp file
		19		25nov25	add color palettes and cycling
        20      27nov25	add submarine transition
		21		28nov25	add symmetrical easing
		22		29nov25	fix text bounds; use overhang metrics only
		23		30nov25	add eraser bitmap
		24		01dec25	add transparent background special cases

*/

#include "stdafx.h"
#include "Sloganeer.h"
#include "SloganDraw.h"
#include "Easing.h"
#include "PathStr.h"
#include "SaveObj.h"
#include "D2DOffscreen.h"

#define DTF(x) static_cast<float>(x)

#define CHECK(x) { HRESULT hr = x; if (FAILED(hr)) { OnError(hr, __FILE__, __LINE__, __DATE__); return false; }}

const float CSloganDraw::m_fSpaceWidth = 0.001f;

CSloganDraw::CSloganDraw()
{
	Init();
}

CSloganDraw::CSloganDraw(const CSloganParams& params) : CSloganParams(params)
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
	m_bIsRecording = false;
	m_iState = ST_TRANS_OUT;
	m_iSlogan = 0;
	m_iTransType = 0;
	m_fTransDuration = 2.0f;
	m_fTileSize = 0;
	m_szTileLayout = CSize(0, 0);
	m_ptTileOffset = CD2DPointF(0, 0);
	m_bIsGlyphRising = false;
	m_bIsFirstGlyphRun = false;
	m_bTransparentBkgnd = false;
	m_iGlyphLine = 0;
	m_iFrame = 0;
	m_nSwapChainBuffers = 0;
	m_fStateStartTime = 0;
	m_fMeltMaxStroke = 0;
	m_nCharsTyped = 0;
	m_fClockRadius = 0;
}

bool CSloganDraw::Create(HWND hWnd)
{
#if SD_CAPTURE >= SD_CAPTURE_MAKE_REF_IMAGES	// if regression testing
	RegressionTestSetup();	// override parameters for testing
#endif	// SD_CAPTURE
	if (m_aSlogan.IsEmpty())	// at least one slogan required
		return false;
	if (!m_evtWake.Create(NULL, false, false, NULL))	// create wake event
		return false;
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

class CSloganOffscreen : public CD2DOffscreen {
	virtual void OnError(HRESULT hr, LPCSTR pszSrcFileName, int nLineNum, LPCSTR pszSrcFileDate) {
		theApp.OnError(hr, pszSrcFileName, nLineNum, pszSrcFileDate);
	}
};

bool CSloganDraw::Record()
{
	CSloganOffscreen	offScreen;
	if (!offScreen.Create(CD2DSizeU(m_szRecFrameSize)))
		return false;
	// set recording state to true; restored automatically when we exit
	CSaveObj<bool>	saveRecording(m_bIsRecording, true);
	// use CopyTo instead of Attach because we don't own these objects,
	// we're only borrowing them; CopyTo adds a reference to the object
	offScreen.m_pD2DFactory.CopyTo(&m_pD2DFactory);
	offScreen.m_pD2DDeviceContext.CopyTo(&m_pD2DDeviceContext);
	if (!CreateUserResources())
		return false;
	if (!OnThreadCreate())
		return false;
	UINT	nRecFrames = Round(m_fRecDuration * m_fRecFrameRate);
	for (m_iFrame = 0; m_iFrame < nRecFrames; m_iFrame++) {	// for each frame
		offScreen.m_pD2DDeviceContext->BeginDraw();	// begin draw
		bool	bDrawResult = OnDraw();	// draw slogan
		CHECK(offScreen.m_pD2DDeviceContext->EndDraw());	// end draw
		if (!bDrawResult)	// if drawing failed
			return false;	// error already handled
		CString	sImagePath(MakeImageSequenceFileName(m_sRecFolderPath, m_iFrame));
		if (!offScreen.Write(sImagePath))	// write frame to file
			return false;
	}
	return true;
}

CString	CSloganDraw::MakeImageSequenceFileName(CString sFolderPath, UINT iFrame)
{
	CString	sSeqNum;
	sSeqNum.Format(_T("%06d"), iFrame);
	CPathStr	sImagePath(sFolderPath);
	sImagePath.Append(_T("\\img") + sSeqNum + _T(".png"));
	return sImagePath;
}

#if SD_CAPTURE	// if capturing frames
void CSloganDraw::CMyD2DCapture::OnError(HRESULT hr, LPCSTR pszSrcFileName, int nLineNum, LPCSTR pszSrcFileDate)
{
	theApp.OnError(hr, pszSrcFileName, nLineNum, pszSrcFileDate);
}
#endif	// SD_CAPTURE

void CSloganDraw::OnError(HRESULT hr, LPCSTR pszSrcFileName, int nLineNum, LPCSTR pszSrcFileDate)
{
	theApp.OnError(hr, pszSrcFileName, nLineNum, pszSrcFileDate);
}

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
	m_pEraserBitmap.Release();
	m_pPathGeom.Release();
	if (m_bThreadExit)	// if thread exiting
		CoUninitialize();	// needed for WIC
}

bool CSloganDraw::OnThreadCreate()
{
	CHECK(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED));	// needed for WIC
	srand(m_nRandSeed);	// initialize random number generator
	m_rlSloganIdx.Init(m_aSlogan.GetSize());	// init slogan index shuffler
	m_rlTransType.Init(TRANS_TYPES);	// init transition type shuffler
	if (m_bSeqSlogans) {	// if showing slogans sequentially
		m_iSlogan = -1;	// avoid skipping first slogan
	}
	LaunchMeltWorker();
	m_fThreadStartTime = m_timerTrans.Time();
	StartSlogan();	// start the first slogan
#if SD_CAPTURE	// if capturing frames
	if (!SetCapture())
		return false;
#endif	// SD_CAPTURE
	return true;
}

void CSloganDraw::OnResize()
{
	OnTextChange();
	// if we're transitioning
	if (m_iState == ST_TRANS_IN || m_iState == ST_TRANS_OUT) {
		// any transition type that initializes itself at 
		// start of transition may require handling here
		switch (m_iTransType) {
		case TT_EXPLODE:
		case TT_RAND_TILE:
			m_bIsTransStart = true;	// redo initialization
			break;
		case TT_RAND_TYPE:
			m_nCharsTyped = 0;	// reset chars typed count
			break;
		}
		m_pEraserBitmap.Release();	// recreate eraser bitmap if needed
	}
}

bool CSloganDraw::OnDraw()
{
	// if background color has a palette and isn't customized
	if (!m_palBkgnd.IsEmpty() && !IsCustomized(COL_bgclr)) {
		UpdateColor(m_palBkgnd, m_clrBkgnd, m_pBkgndBrush);	// update background color and brush
	}
	// if draw color has a palette and isn't customized
	if (!m_palDraw.IsEmpty() && !IsCustomized(COL_drawclr)) {
		UpdateColor(m_palDraw, m_clrDraw, m_pDrawBrush);	// update draw color and brush
	}
	m_pD2DDeviceContext->Clear(m_clrBkgnd);	// clear frame to background color
	double	fTransElapsed;	// elapsed time since transition started
	if (m_bIsRecording) {	// if recording
		fTransElapsed = GetFrameTime() - m_fStateStartTime;	// compute time from frame index
	} else {	// not recording
		fTransElapsed = m_timerTrans.Elapsed();	// get elapsed time from transition timer
	}
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
	case TT_RAND_TYPE:
		TransRandomTypewriter();
		break;
	case TT_FADE:
		TransFade();
		break;
	case TT_SCALE_HORZ:
	case TT_SCALE_VERT:
	case TT_SCALE_BOTH:
	case TT_SCALE_SPIN:
		TransScale();
		break;
	case TT_RAND_TILE:
		TransRandTile();
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
	case TT_SKEW:
		TransSkew();
		break;
	case TT_EXPLODE:
		TransExplode();
		break;
	case TT_SUBMARINE:
		TransSubmarine();
		break;
	default:
		NODEFAULTCASE;	// logic error
	}
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
	m_iFrame++;
#endif	// SD_CAPTURE
	return true;
}

HRESULT CSloganDraw::DrawGlyphRun(void* pClientDrawingContext, FLOAT fBaselineOriginX, 
	FLOAT fBaselineOriginY, DWRITE_MEASURING_MODE measuringMode, DWRITE_GLYPH_RUN const* pGlyphRun, 
	DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, IUnknown* pClientDrawingEffect)
{
	UINT	nGlyphs = pGlyphRun->glyphCount;
	m_aGlyphOffset.SetSize(nGlyphs);	// array is member to reduce reallocation
	if (nGlyphs > 0) {	// if one or more glyphs
		if (pGlyphRun->glyphOffsets != NULL)	// if caller specified glyph offsets, copy them
			memcpy(m_aGlyphOffset.GetData(), pGlyphRun->glyphOffsets, sizeof(DWRITE_GLYPH_OFFSET) * nGlyphs);
		else {	// no glyph offsets, so zero our array
			ZeroMemory(m_aGlyphOffset.GetData(), sizeof(DWRITE_GLYPH_OFFSET) * nGlyphs);
		}
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
	case TT_SKEW:
		TransSkew(ptBaselineOrigin, measuringMode, pGlyphRunDescription, &glyphRun);
		break;
	case TT_EXPLODE:
		TransExplode(ptBaselineOrigin, measuringMode, pGlyphRunDescription, &glyphRun);
		break;
	case TT_SUBMARINE:
		TransSubmarine(ptBaselineOrigin, measuringMode, pGlyphRunDescription, &glyphRun);
		break;
	default:
		NODEFAULTCASE;
	}
	return S_OK;
}

void CSloganDraw::StartSlogan()
{
	if (m_bSeqSlogans) {	// if showing slogans sequentially
		m_iSlogan++;	// next slogan index
		if (m_iSlogan >= m_aSlogan.GetSize())	// if reached end of slogans
			m_iSlogan = 0;	// reset to first slogan
	} else {	// randomizing slogans
		m_iSlogan = m_rlSloganIdx.GetNext(m_iSlogan);	// get next slogan index
	}
	m_sSlogan = m_aSlogan[m_iSlogan].m_sText;	// cache current slogan
	if (m_bCustomSlogans)	// if doing per-slogan customization
		OnCustomSlogan();	// customize current slogan
	OnTextChange();	// update text
	m_bTransparentBkgnd = m_clrBkgnd.a == 0.0f;	// true if alpha is zero
	StartTrans(ST_TRANS_IN, m_fInTransDuration);	// start incoming transition
}

void CSloganDraw::StartTrans(int nState, float fDuration)
{
	ASSERT(nState == ST_TRANS_IN || nState == ST_TRANS_OUT);	// else logic error
	if (nState == ST_TRANS_OUT)	{	// if starting outgoing transition
		// existing layout instance is reused, so any layout changes made
		// by incoming transition, such as drawing effects, must be reset
		switch (m_iTransType) {	// previous transition type
		case TT_TYPEWRITER:
		case TT_RAND_TYPE:
			ResetDrawingEffect();	// remove per-character brushes
			break;
		}
	}
	m_iState = nState;	// set state
	m_fTransDuration = fDuration;
	m_bIsTransStart = true;	// set transition start flag
	if (m_bIsRecording) {	// if recording
		m_fStateStartTime = GetFrameTime();	// compute time from frame index
	} else {	// not recording
		m_timerTrans.Reset();	// reset transition timer
	}
	int	iTransDir = nState == ST_TRANS_OUT;	// get transition direction
	// if current slogan specifies a transition type override for this direction
	if (IsValid(m_aTransType[iTransDir])) {
		m_iTransType = m_aTransType[iTransDir];	// use specified transition type
	} else {	// transition type isn't overridden
		m_iTransType = m_rlTransType.GetNext(m_iTransType);	// get next transition type
	}
	ResetTransform(m_pD2DDeviceContext);	// remove transform if any
	m_pEraserBitmap.Release();	// recreate eraser bitmap if needed
}

void CSloganDraw::StartIdle(int nDuration)
{
	if (m_bIsRecording) {	// if recording
		// compute time from frame index
		m_nWakeTime = Round64(GetFrameTime() * 1000 + nDuration);
	} else {	// not recording
		m_nWakeTime = GetTickCount64() + nDuration;	// set wake time
	}
}

bool CSloganDraw::ContinueIdle()
{
	if (m_bIsRecording) {	// if recording
		ULONGLONG	nNow = Round64(GetFrameTime() * 1000);
		return nNow >= m_nWakeTime;
	}
	ULONGLONG	nNow = GetTickCount64();
	if (nNow < m_nWakeTime) {	// if more idle time remains
#if SD_CAPTURE	// if capturing frames
		return false;	// draw during idle, so idle gets captured
#else	// not capturing frames
		// if either palette is non-empty, assume color cycling
		if (!m_palBkgnd.IsEmpty() || !m_palDraw.IsEmpty())
			return false;	// color cycle during idle
		// block instead of drawing, to reduce power usage
		DWORD	nTimeout = static_cast<DWORD>(m_nWakeTime - nNow);
		// wait for wake signal or timeout
		DWORD	nRet = WaitForSingleObject(m_evtWake, nTimeout);
		if (nRet == WAIT_OBJECT_0)	// if wake signal
			return false;	// idle is incomplete
#endif	// SD_CAPTURE
	}
	return true;	// idle is over
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

void CSloganDraw::UpdateColor(CPalette& palette, D2D1::ColorF& color, ID2D1SolidColorBrush* pBrush)
{
	double	fElapsedTime;
	if (m_bIsRecording) {	// if recording
		fElapsedTime = GetFrameTime();	// compute time from frame index
	} else {	// not recording
		fElapsedTime = m_timerTrans.Time() - m_fThreadStartTime;	// get time from timer
	}
	palette.CycleColor(fElapsedTime, color);	// cycle color based on elapsed time
	pBrush->SetColor(color);	// update brush color
}

void CSloganDraw::OnCustomSlogan()
{
	CSlogan	prevSlogan(*this);	// copy current slogan for change detection
	CSlogan::operator=(m_aSlogan[m_iSlogan]);	// update this slogan from slogan array
	if (!IsSameFont(prevSlogan)) {	// if font changed
		OnFontChange();	// update font
	}
	if (!IsSameColor(m_clrBkgnd, prevSlogan.m_clrBkgnd)) {	// if background color changed
		m_pBkgndBrush->SetColor(m_clrBkgnd);	// set background color 
	}
	if (!IsSameColor(m_clrDraw, prevSlogan.m_clrDraw)) {	// if draw color changed
		m_pDrawBrush->SetColor(m_clrDraw);	// set draw color 
	}
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
	// assuming text fits within the frame, overhang metrics are
	// NEGATIVE distances in DIPs from frame edge to text edge
	rText = CKD2DRectF(
		-m_overhangMetrics.left,
		-m_overhangMetrics.top, 
		szRT.width + m_overhangMetrics.right,
		szRT.height + m_overhangMetrics.bottom
	);
	rText.InflateRect(AA_MARGIN, AA_MARGIN);	// add antialiasing margin
	return szRT;	// return render target size
}

void CSloganDraw::DrawTextBounds()
{
	CKD2DRectF	rText;
	GetTextBounds(rText);
	m_pD2DDeviceContext->DrawRectangle(rText, m_pDrawBrush);
}

void CSloganDraw::DrawGlyphBounds(CD2DPointF ptBaselineOrigin, DWRITE_GLYPH_RUN const* pGlyphRun,
	bool bTightVertBounds)
{
	CKD2DRectF	rGlyph;
	UINT	iGlyph;
	CGlyphIter	iterGlyph(ptBaselineOrigin, pGlyphRun, bTightVertBounds);
	// bounding boxes can overlap; alternating between two colors helps distinguish them
	static const D2D1_COLOR_F	aClr[2] = {{0, 1, 0, 1}, {1, 0, 0, 1}};	// green and red
	while (iterGlyph.GetNext(iGlyph, rGlyph)) {	// for each glyph
		m_pVarBrush->SetColor(aClr[iGlyph & 1]);	// alternate colors
		m_pD2DDeviceContext->DrawRectangle(rGlyph, m_pVarBrush);
	}
	if (pGlyphRun->glyphCount) {
		CD2DSizeF	szRT = m_pD2DDeviceContext->GetSize();
		m_pD2DDeviceContext->DrawLine(CD2DPointF(0, ptBaselineOrigin.y),
			CD2DPointF(szRT.width, ptBaselineOrigin.y), m_pDrawBrush);	// draw baseline too
	}
}

void CSloganDraw::GetRunBounds(CKD2DRectF& rRun, CD2DPointF ptBaselineOrigin, 
	DWRITE_GLYPH_RUN const* pGlyphRun, bool bTightVertBounds) const
{
	CGlyphIter	iterGlyph(ptBaselineOrigin, pGlyphRun, bTightVertBounds);
	CKD2DRectF	rGlyph;
	UINT	iGlyph;
	if (iterGlyph.GetNext(iGlyph, rRun)) {	// set destination to first glyph's rectangle
		while (iterGlyph.GetNext(iGlyph, rGlyph)) {	// for each remaining glyph
			rRun.Union(rGlyph);	// union destination with this glyph's rectangle
		}
	}
}

double CSloganDraw::GetPhase(UINT nFlags) const
{
	bool	bOutgoing = IsTransOut();
	double	fPhase;	// normalized position from 0 to 1
	if (nFlags & GP_EASING) {	// if easing enabled
		if (nFlags & GP_EASE_BOTH) {	// if easing both in and out
			// symmetrical easing
			fPhase = EaseInAndOut(m_fTransProgress, m_fEasing);
		} else {
			// if incoming transition, ease out, otherwise ease in
			fPhase = EaseInOrOut(!bOutgoing, m_fTransProgress, m_fEasing);
		}
	} else {	// easing disabled
		fPhase = m_fTransProgress;	// linear
	}
	bool	bInvert = nFlags & GP_INVERT;
	if (bOutgoing == bInvert)	// if transition should be reversed
		fPhase = 1 - fPhase;	// invert phase
	return fPhase;
}

double CSloganDraw::GetFrameRate()
{
	if (m_bIsRecording) {	// if recording
		return m_fRecFrameRate;	// use record frame rate
	} else {	// not recording
		DWORD	dwDisplayFreq;
		if (!GetDisplayFrequency(dwDisplayFreq))	// use display frequency
			return 0;	// failed to obtain display frequency
		return dwDisplayFreq;
	}
}

double CSloganDraw::GetFrameTime() const
{
	// during recording, compute time from frame index to avoid jitter
	return m_iFrame * (1.0 / m_fRecFrameRate);
}

bool CSloganDraw::CreateEraser(CD2DSizeF& szMask, FLOAT fAlpha)
{
	// this method only supports a fully transparent background, with alpha of zero;
	// partial transparency will cause the mask to be visible against the background
	ASSERT(szMask.width && szMask.height);	// mask must have non-zero size
	m_pEraserBitmap.Release();	// just in case
	D2D1_SIZE_F	szDpi = GetDpi(m_pD2DDeviceContext);	// only call GetDpi once
	D2D1_BITMAP_PROPERTIES1 props =
		D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET,
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
		szDpi.width, szDpi.height);	// specify target DPI when creating bitmap
	CD2DSizeU	szTextPixels(DipsToPixels(m_pD2DDeviceContext, szMask, szDpi));
	CHECK(m_pD2DDeviceContext->CreateBitmap(szTextPixels, NULL, 0, props, &m_pEraserBitmap));
	CComPtr<ID2D1Image>	pOldTarget;	// smart pointer is required due to reference counting
	m_pD2DDeviceContext->GetTarget(&pOldTarget);	// increments target's reference count
	m_pD2DDeviceContext->SetTarget(m_pEraserBitmap);	// set target to eraser bitmap
	m_pD2DDeviceContext->Clear(D2D1::ColorF(0, 0, 0, fAlpha));	// erase at requested strength
	m_pD2DDeviceContext->SetTarget(pOldTarget);	// restore previous target
	return true;
}

void CSloganDraw::EraseBackground(D2D1_POINT_2F ptTargetOffset, D2D1_RECT_F *pImageRect)
{
	ASSERT(m_pEraserBitmap != NULL);	// sanity check
	// offset is in world DIPs, rect is in DIPs relative to bitmap's top left
	// corner; if rect is smaller than bitmap, bitmap is cropped, not resized
	m_pD2DDeviceContext->DrawImage(m_pEraserBitmap, &ptTargetOffset, pImageRect, 
		D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR,	// avoid antialiasing
		D2D1_COMPOSITE_MODE_DESTINATION_OUT);	// bitmap acts as an eraser
}

#if SD_CAPTURE	// if capturing frames
bool CSloganDraw::SetCapture(bool bEnable)
{
	if (bEnable == m_capture.IsCreated())	// if state unchanged
		return true;	// nothing to do
	if (bEnable) {	// if starting capture
		// full screen mode must be established before capture instance is created,
		// so process pending render commands, in case full screen is one of them
		CRenderCmd	cmd;
		while (m_qCmd.Pop(cmd)) {	// while commands remain
			ProcessCommand(cmd);	// process command
		}
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
	if (m_iFrame < m_nSwapChainBuffers)	// if initial blank frame
		return true;	// skip blank frame; not an error
	if (!m_capture.CaptureFrame())	// if capture frame fails
		m_capture.Destroy();	// abort capture
	return true;
}
#endif	// SD_CAPTURE

#if SD_CAPTURE >= SD_CAPTURE_MAKE_REF_IMAGES	// if regression testing
void CSloganDraw::RegressionTestSetup()
{
	CSloganParams	params;
	CSloganParams::operator=(params);	// set default parameters
	CSlogan	slogan;
	slogan.m_sText = L"Your Text\n"	// 2nd line is hello world in Arabic
		L"\x0645\x0631\x062D\x0628\x0627 "
		L"\x0628\x0627\x0644\x0639\x0627\x0644\x0645";
	slogan.m_sFontName = L"Times New Roman";
	slogan.m_fFontSize = 150.0f;
	slogan.m_nFontWeight = DWRITE_FONT_WEIGHT_BLACK;
	slogan.m_clrBkgnd = D2D1::ColorF::Black;
	slogan.m_clrDraw = D2D1::ColorF::White;
	m_bCustomSlogans = true;	// enable customization
	m_aSlogan.Add(slogan);
}

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
	if (m_iFrame == nEndFrame) {	// if last frame captured
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
	const bool	bIsOddFrame = m_iFrame & 1;
	const int	iState = bIsOddFrame ? ST_TRANS_OUT : ST_TRANS_IN;
	StartTrans(iState, 1);	// start a new transition
	// override transition type
	if (bMakeRefs) {	// if generating reference frames
		m_iTransType = (m_iFrame / 2) % TRANS_TYPES;
	} else {	// checking permutations against reference
		m_iTransType = bIsOddFrame ? (m_iFrame / 2) % TRANS_TYPES 
			: m_iFrame / REF_FRAMES % TRANS_TYPES;
	}
	m_fTransProgress = 1.0 / 3;	// override transition progress
	m_nHoldDuration = 0;	// no hold
	m_nPauseDuration = 0;	// no pause either
	srand(0);	// ensure consistent behavior for transitions that use randomness
	return true;
}
#endif	// SD_CAPTURE
