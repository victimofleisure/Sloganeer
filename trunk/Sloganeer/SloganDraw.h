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
		08		10nov25	add regression test
		09		11nov25	add elevator and clock transitions
		10		12nov25	add skew and spin transitions
		11		14nov25	add recording
		12		17nov25	add explode transition
		13		18nov25	add per-slogan customization
		14		22nov25	add random typewriter variant

*/

#pragma once

#include "RenderThread.h"
#include "Event.h"
#include "RandList.h"
#include "SloganParams.h"
#include "D2DHelper.h"
#include "MeltProbe.h"
#include "TriangleSink.h"

#define	SD_CAPTURE_NONE				0	// disable capture
#define	SD_CAPTURE_RECORD			1	// record displayed frames as an image sequence
#define	SD_CAPTURE_MAKE_REF_IMAGES	2	// generate reference images for regression test
#define	SD_CAPTURE_REGRESSION_TEST	3	// regression test against saved reference images

#define SD_CAPTURE SD_CAPTURE_NONE	// one of the above values

#if SD_CAPTURE
#include "D2DCapture.h"
#endif	// SD_CAPTURE

class CSloganDraw : public CRenderThread, protected CSloganParams, protected CTextRenderer {
public:
// Construction
	CSloganDraw();
	CSloganDraw(const CSloganParams& params);
	~CSloganDraw();
	bool	Create(HWND hWnd);
	void	Destroy();

// Attributes
	bool	IsFullScreen() const;
	const CSloganParams&	GetParams() const;

// Operations
	void	Resize();
	bool	FullScreen(bool bEnable);
	bool	Record();
	static CString	MakeImageSequenceFileName(CString sFolderPath, UINT iFrame);

protected:
// Constants
	enum {	// display states
		ST_TRANS_IN,	// incoming transition
		ST_HOLD,		// freeze slogan
		ST_TRANS_OUT,	// outgoing transition
		ST_PAUSE,		// pause between slogans
		STATES
	};
	enum {	// GetPhase flags
		GP_INVERT	= 0x01,		// invert phase
		GP_EASING	= 0x02,		// apply easing
	};
	enum {
		AA_MARGIN = 1,	// extra margin to account for antialiasing
	};

// Types
	typedef CArrayEx<DWRITE_GLYPH_OFFSET, DWRITE_GLYPH_OFFSET&> CGlyphOffsetArray;
	typedef CArrayEx<DWRITE_LINE_METRICS, DWRITE_LINE_METRICS&> CLineMetricsArray;

// Members
	CComPtr<ID2D1SolidColorBrush>	m_pBkgndBrush;	// background brush interface
	CComPtr<ID2D1SolidColorBrush>	m_pDrawBrush;	// drawing brush interface
	CComPtr<ID2D1SolidColorBrush>	m_pVarBrush;	// variable brush interface
	CComPtr<IDWriteFactory>	m_pDWriteFactory;	// DirectWrite factory interface
	CComPtr<IDWriteTextFormat>	m_pTextFormat;	// text format interface
	CComPtr<IDWriteTextLayout>	m_pTextLayout;	// text layout interface
	CComPtr<ID2D1StrokeStyle1>	m_pStrokeStyle;	// stroke style for melt effect
	DWRITE_TEXT_METRICS	m_textMetrics;	// text metrics
	DWRITE_OVERHANG_METRICS	m_overhangMetrics;	// overhang metrics
	WEvent	m_evtWake;			// during hold, signals wake from idle
	CRandList	m_rlTransType;	// randomized list of transition types
	CRandList	m_rlSloganIdx;	// randomized list of slogan indices
	CBenchmark	m_timerTrans;	// high-performance transition timer
	ULONGLONG	m_nWakeTime;	// during hold, wake time in CPU ticks
	CString		m_sSlogan;		// current slogan being displayed
	double	m_fTransProgress;	// transition progress, normalized from 0 to 1
	bool	m_bThreadExit;		// true if render thread exit requested
	bool	m_bIsFullScreen;	// true if in full screen mode
	bool	m_bIsTransStart;	// true if starting a transition
	bool	m_bIsRecording;		// true if recording a video
	int		m_iState;			// index of current state
	int		m_iSlogan;			// index of current slogan
	int		m_iTransType;		// index of current transition type
	float	m_fTransDuration;	// duration of current transition, in seconds
	float	m_fTileSize;		// tile size, in DIPs
	CSize	m_szTileLayout;		// tiling layout, in rows and columns
	CD2DPointF	m_ptTileOffset;	// tile offset, in DIPs
	CIntArrayEx	m_aTileIdx;		// array of tile indices
	CGlyphOffsetArray	m_aGlyphOffset;	// array of glyph offsets, for text renderer
	CLineMetricsArray	m_aLineMetrics;	// array of line metrics, for text renderer
	bool	m_bIsGlyphRising;	// true if glyph is rising; for vertical converge
	int		m_iGlyphLine;		// index of line text renderer is currently on
	CIntArrayEx	m_aCharToLine;	// for each character of slogan, index of its line

	// recording and capture
	UINT	m_iFrame;			// frame counter
	UINT	m_nSwapChainBuffers;	// number of swap chain buffers
	double	m_fStartTime;		// start time of current state in seconds

	// melt transition
	float	m_fMeltMaxStroke;	// maximum outline stroke for melt effect, in DIPs
	CMeltProbeWorker	m_thrMeltWorker;	// melt probe worker thread instance
	CArrayEx<float, float>	m_aMeltStroke;	// array of cached melt outline strokes

	// explode transition
	CTriangleSink	m_triSink;	// triangle sink containing array of triangles

	// random typewriter transition
	CIntArrayEx	m_aCharIdx;		// array of character indices within current slogan
	UINT	m_nCharsTyped;		// count of characters that have been typed so far
	int		m_iTransVariant;	// index of transition variant; zero for default

#if SD_CAPTURE	// if capturing frames
	class CMyD2DCapture : public CD2DCapture {
	public:
		virtual void	OnError(HRESULT hr, LPCSTR pszSrcFileName, int nLineNum, LPCSTR pszSrcFileDate);
	};
	CMyD2DCapture	m_capture;	// frame capture instance
#endif	// SD_CAPTURE

// Overrides
	virtual void	OnError(HRESULT hr, LPCSTR pszSrcFileName, int nLineNum, LPCSTR pszSrcFileDate);
	virtual	bool	CreateUserResources();
	virtual	void	DestroyUserResources();
	virtual	bool	OnThreadCreate();
	virtual	void	OnResize();
	virtual	bool	OnDraw();

// COM overrides
	// AddRef and Release should never be called, but overload them safely anyway
	IFACEMETHOD_(ULONG, AddRef)() { return 1; }
	IFACEMETHOD_(ULONG, Release)() { return 1; }
    IFACEMETHOD(DrawGlyphRun)(void* pClientDrawingContext, FLOAT fBaselineOriginX, 
		FLOAT fBaselineOriginY, DWRITE_MEASURING_MODE measuringMode, DWRITE_GLYPH_RUN const* pGlyphRun, 
		DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, IUnknown* pClientDrawingEffect);

// Helpers
	void	Init();
	bool	IsTransOut() const;
	void	OnCustomSlogan();
	void	StartSlogan();
	void	StartTrans(int nState, float fDuration);
	void	StartIdle(int nDuration);
	bool	ContinueIdle();
	bool	OnFontChange();
	bool	OnTextChange();
	CD2DSizeF	GetTextBounds(CKD2DRectF& rText) const;
	void	DrawTextBounds();
	void	DrawGlyphBounds(CD2DPointF ptBaselineOrigin, DWRITE_GLYPH_RUN const* pGlyphRun);
	double	GetPhase(UINT nFlags = 0) const;
	double	GetFrameRate();
	bool	CreateStrokeStyle();
	bool	ResetDrawingEffect();
	void	TransScroll();
	void	TransReveal();
	void	TransTypewriter();
	void	TransRandomTypewriter();
	void	TransFade();
	void	TransScale();
	void	TransTile();
	void	InitTiling(const CKD2DRectF& rText);
	bool	TransConverge();
	void	TransConvergeHorz(CD2DPointF ptBaselineOrigin, DWRITE_MEASURING_MODE measuringMode, 
		DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, DWRITE_GLYPH_RUN const* pGlyphRun);
	void	TransConvergeVert(CD2DPointF ptBaselineOrigin, DWRITE_MEASURING_MODE measuringMode, 
		DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, DWRITE_GLYPH_RUN const* pGlyphRun);
	bool	TransMelt();
	bool	TransMelt(CD2DPointF ptBaselineOrigin, DWRITE_MEASURING_MODE measuringMode, 
		DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, DWRITE_GLYPH_RUN const* pGlyphRun);
	bool	TransElevator();
	void	TransElevator(CD2DPointF ptBaselineOrigin, DWRITE_MEASURING_MODE measuringMode, 
		DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, DWRITE_GLYPH_RUN const* pGlyphRun);
	bool	TransClock();
	bool	TransClock(CD2DPointF ptBaselineOrigin, DWRITE_MEASURING_MODE measuringMode, 
		DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, DWRITE_GLYPH_RUN const* pGlyphRun);
	bool	TransSkew();
	void	TransSkew(CD2DPointF ptBaselineOrigin, DWRITE_MEASURING_MODE measuringMode, 
		DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, DWRITE_GLYPH_RUN const* pGlyphRun);
	bool	TransExplode();
	bool	TransExplode(CD2DPointF ptBaselineOrigin, DWRITE_MEASURING_MODE measuringMode, 
		DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, DWRITE_GLYPH_RUN const* pGlyphRun);
	bool	LaunchMeltWorker();
	bool	MeasureMeltStroke();
	bool	GetLineMetrics();
	bool	MakeCharToLineTable();
	bool	SetCapture(bool bEnable = true);
	bool	CaptureFrame();
	bool	RegressionTest();
	double	GetFrameTime() const;
	static double	Lerp(double a, double b, double t);
};

inline bool CSloganDraw::IsTransOut() const
{
	return m_iState == ST_TRANS_OUT || m_iState == ST_PAUSE;
}

inline bool CSloganDraw::IsFullScreen() const
{
	return m_bIsFullScreen;
}

inline const CSloganParams& CSloganDraw::GetParams() const
{
	return *this;	// automatic upcast
}
