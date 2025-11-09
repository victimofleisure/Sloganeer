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

*/

#pragma once

#include "RenderThread.h"
#include "Event.h"
#include "RandList.h"
#include "Benchmark.h"
#include "D2DHelper.h"
#include "SloganParams.h"
#include "MeltProbe.h"

#define CAPTURE_FRAMES 0	// non-zero to capture frames

#if CAPTURE_FRAMES
#include "D2DCapture.h"
#endif	// CAPTURE_FRAMES

class CSloganDraw : public CRenderThread, protected CSloganParams, protected CTextRenderer {
public:
// Construction
	CSloganDraw();
	CSloganDraw(CSloganParams& params);
	~CSloganDraw();
	bool	Create(HWND hWnd);
	void	Destroy();

// Attributes
	bool	IsFullScreen() const;
	const CSloganParams&	GetParms() const;

// Operations
	void	Resize();
	bool	FullScreen(bool bEnable);

protected:
// Constants
	enum {	// display states
		ST_TRANS_IN,	// incoming transition
		ST_HOLD,		// freeze slogan
		ST_TRANS_OUT,	// outgoing transition
		ST_PAUSE,		// pause between slogans
		STATES
	};
	enum {	// transition types
		TT_SCROLL_LR,	// scroll from left to right
		TT_SCROLL_RL,	// scroll from right to left
		TT_SCROLL_TB,	// scroll from top to bottom
		TT_SCROLL_BT,	// scroll from bottom to top
		TT_REVEAL_LR,	// reveal or cover from left to right
		TT_REVEAL_TB,	// reveal or cover from top to bottom
		TT_TYPEWRITER,	// reveal or cover one letter at a time
		TT_FADE,		// fade to or from background color
		TT_SCALE_HORZ,	// scale horizontally
		TT_SCALE_VERT,	// scale vertically
		TT_SCALE_BOTH,	// scale both axes
		TT_TILE,		// reveal or cover with tiles
		TT_CONVERGE_HORZ,	// converge horizontally
		TT_CONVERGE_VERT,	// converge vertically
		TT_MELT,		// outline with increasing stroke width
		TRANS_TYPES
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
	float	m_fMeltMaxStroke;	// maximum outline stroke for melt effect, in DIPs
	CMeltProbeWorker	m_thrMeltWorker;	// melt probe worker thread instance
	CArrayEx<float, float>	m_aMeltStroke;	// array of cached melt outline strokes

#if CAPTURE_FRAMES	// if capturing frames
	class CMyD2DCapture : public CD2DCapture {
	public:
		virtual void	OnError(HRESULT hr, LPCSTR pszSrcFileName, int nLineNum, LPCSTR pszSrcFileDate);
		CSloganDraw	*m_pParent;	// pointer to parent instance
	};
	CMyD2DCapture	m_capture;	// frame capture instance
#endif	// CAPTURE_FRAMES

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
	void	StartSlogan();
	void	StartTrans(int nState, float fDuration);
	void	StartIdle(int nDuration);
	bool	ContinueIdle();
	bool	OnFontChange();
	bool	OnTextChange();
	CD2DSizeF	GetTextBounds(CKD2DRectF& rText) const;
	bool	CreateStrokeStyle();
	bool	ResetDrawingEffect();
	void	TransScroll();
	void	TransReveal();
	void	TransTypewriter();
	void	TransFade();
	void	TransScale();
	void	TransTile();
	void	InitTiling(const CKD2DRectF& rText);
	void	TransConverge();
	void	TransConvergeHorz(CD2DPointF ptBaselineOrigin, DWRITE_MEASURING_MODE measuringMode, 
		DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, DWRITE_GLYPH_RUN const* pGlyphRun);
	void	TransConvergeVert(CD2DPointF ptBaselineOrigin, DWRITE_MEASURING_MODE measuringMode, 
		DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, DWRITE_GLYPH_RUN const* pGlyphRun);
	void	TransMelt();
	bool	TransMelt(CD2DPointF ptBaselineOrigin, DWRITE_MEASURING_MODE measuringMode, 
		DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, DWRITE_GLYPH_RUN const* pGlyphRun);
	bool	LaunchMeltWorker();
	bool	MeasureMeltStroke();
	bool	GetLineMetrics();
	bool	MakeCharToLineTable();
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

inline const CSloganParams& CSloganDraw::GetParms() const
{
	return *this;	// automatic upcast
}
