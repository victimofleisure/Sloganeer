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
		15		25nov25	add color palettes and cycling
        16      27nov25	add submarine transition
		17		28nov25	add symmetrical easing
		18		30nov25	add eraser bitmap
		19		01dec25	add transparent background special cases
		20		11dec25	add commands for UI
		21		14dec25	add manual trigger
		22		15dec25	add tumble transition
		23		20dec25	add iris transition
		24		21dec25	remove unused member vars
		25		24dec25	add command to set slogan array
		26		27dec25	add glyph run callback member function pointer
		27		28dec25	merge typewriter effects

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
	bool	Create(HWND hWnd, CWnd* pNotifyWnd = NULL);
	void	Destroy();

// Attributes
	bool	IsFullScreen() const;
	const CSloganParams&	GetParams() const;

// Commands
	void	SetSlogan(const CSlogan& slogan);
	void	SetSloganArray(const CSloganArray& aSlogan, bool bHasCustom);
	void	InsertSlogan(int iSlogan, const CSlogan& slogan);
	void	SetSloganText(int iSlogan, CString sSlogan);
	void	SelectSlogan(int iSlogan);
	void	SetCustomSlogans(bool bEnable);
	void	SetImmediateMode(bool bEnable);
	void	SetSloganPlayMode(int iPlayMode);
	void	SetTriggerType(bool bIsManual);
	void	TriggerGo();

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
		GP_INVERT		= 0x01,		// invert phase
		GP_EASING		= 0x02,		// apply easing
		GP_EASE_BOTH	= 0x04,		// symmetrical easing, in and out
	};
	enum {
		AA_MARGIN = 1,	// extra margin to account for antialiasing
	};
	static const float m_fSpaceWidth;	// glyph this wide or less is a space, in DIPs
	enum {	// drawing commands
		RC_SET_SLOGAN = BASE_RENDER_COMMANDS,	// param: none, prop: CSlogan*
		RC_SET_SLOGAN_ARRAY,		// param: bHasCustom, prop: CSloganArray*
		RC_INSERT_SLOGAN,			// param: iSlogan, prop: CSlogan*
		RC_SET_SLOGAN_TEXT,			// param: iSlogan, prop: LPTSTR
		RC_SELECT_SLOGAN,			// param: iSlogan, prop: none
		RC_SET_SLOGAN_PLAY_MODE,	// param: iPlayMode, prop: none
		RC_SET_TRIGGER_TYPE,		// param: bIsManual, prop: none
		RC_TRIGGER_GO,				// param: none, prop: none
	};

// Types
	typedef CArrayEx<DWRITE_GLYPH_OFFSET, DWRITE_GLYPH_OFFSET&> CGlyphOffsetArray;
	typedef CArrayEx<DWRITE_LINE_METRICS, DWRITE_LINE_METRICS&> CLineMetricsArray;
	typedef bool (CSloganDraw::*GlyphRunCallbackPtr)(
		CKD2DPointF ptBaselineOrigin, 
		DWRITE_MEASURING_MODE measuringMode, 
		DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, 
		DWRITE_GLYPH_RUN const* pGlyphRun);

// Data members
	CComPtr<ID2D1SolidColorBrush>	m_pBkgndBrush;	// background brush interface
	CComPtr<ID2D1SolidColorBrush>	m_pDrawBrush;	// drawing brush interface
	CComPtr<ID2D1SolidColorBrush>	m_pVarBrush;	// variable brush interface
	CComPtr<IDWriteFactory>	m_pDWriteFactory;	// DirectWrite factory interface
	CComPtr<IDWriteTextFormat>	m_pTextFormat;	// text format interface
	CComPtr<IDWriteTextLayout>	m_pTextLayout;	// text layout interface
	CComPtr<ID2D1StrokeStyle1>	m_pStrokeStyle;	// stroke style for melt effect
	CComPtr<ID2D1Bitmap1> m_pEraserBitmap;	// for erasing to transparent background
	CComPtr<ID2D1PathGeometry> m_pPathGeom;		// path geometry interface
	CComPtr<ID2D1Layer> m_pLayer;	// for transitions that need a layer
	DWRITE_TEXT_METRICS	m_textMetrics;	// text metrics
	DWRITE_OVERHANG_METRICS	m_overhangMetrics;	// overhang metrics
	CRandList	m_rlTransType;	// randomized list of transition types
	CRandList	m_rlSloganIdx;	// randomized list of slogan indices
	CBenchmark	m_timer;		// high-performance timer
	CString		m_sSlogan;		// current slogan being displayed
	GlyphRunCallbackPtr	m_pGlyphRunCB;	// pointer to current glyph run callback
	double	m_fThreadStartTime;	// when thread was launched, in seconds since boot
	double	m_fTransProgress;	// transition progress, normalized from 0 to 1
	bool	m_bThreadExit;		// true if render thread exit requested
	bool	m_bIsFullScreen;	// true if in full screen mode
	bool	m_bIsTransStart;	// true if starting a transition
	bool	m_bIsRecording;		// true if recording a video
	int		m_iState;			// index of current state
	int		m_iSlogan;			// index of current slogan
	int		m_iTransType;		// index of current transition type
	float	m_fStateDur;		// duration of current state, in seconds
	float	m_fTileSize;		// tile size, in DIPs
	CSize	m_szTileLayout;		// tiling layout, in rows and columns
	CKD2DPointF	m_ptTileOffset;	// tile offset, in DIPs
	CIntArrayEx	m_aTileIdx;		// array of tile indices
	CGlyphOffsetArray	m_aGlyphOffset;	// array of glyph offsets, for text renderer
	CLineMetricsArray	m_aLineMetrics;	// array of line metrics, for text renderer
	CIntArrayEx	m_aCharToLine;	// for each character of slogan, index of its line
	int		m_iGlyphLine;		// index of line text renderer is currently on
	bool	m_bIsGlyphRising;	// true if glyph is rising; for vertical converge
	bool	m_bTransparentBkgnd;	// true if background is fully transparent
	bool	m_bIsImmediateMode;	// true if commands should take effect immediately
	bool	m_bIsManualTrigger;	// true if starting slogans manually; false for auto

	// recording and capture
	UINT	m_iFrame;			// frame counter
	UINT	m_nSwapChainBuffers;	// number of swap chain buffers
	double	m_fStateStartTime;	// start time of current state in seconds

	// melt transition
	float	m_fMeltMaxStroke;	// maximum outline stroke for melt effect, in DIPs
	CMeltProbeWorker	m_thrMeltWorker;	// melt probe worker thread instance
	CArrayEx<float, float>	m_aMeltStroke;	// array of cached melt outline strokes

	// explode transition
	CTriangleSink	m_triSink;	// triangle sink containing array of triangles

	// typewriter transition
	CIntArrayEx	m_aCharIdx;		// array of character indices within current slogan
	UINT	m_nCharsTyped;		// count of characters that have been typed so far

#if SD_CAPTURE	// if capturing frames
	class CMyD2DCapture : public CD2DCapture {
	public:
		virtual void	OnError(HRESULT hr, LPCSTR pszSrcFileName, int nLineNum, LPCSTR pszSrcFileDate);
	};
	CMyD2DCapture	m_capture;	// frame capture instance
#endif	// SD_CAPTURE

// CRenderTarget overrides
	virtual void	OnError(HRESULT hr, LPCSTR pszSrcFileName, int nLineNum, LPCSTR pszSrcFileDate);
	virtual	bool	CreateUserResources();
	virtual	void	DestroyUserResources();
	virtual	bool	OnThreadCreate();
	virtual	void	OnResize();
	virtual	bool	OnDraw();
	virtual void	OnRenderCommand(const CRenderCmd& cmd);

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
	bool	IsIdle() const;
	void	StartSlogan();
	void	StartTrans(int nState, float fDuration);
	void	StartIdle(float fDuration);
	bool	ContinueIdle();
	bool	CreateStrokeStyle();
	bool	ResetDrawingEffect();
	void	UpdateColor(CPalette& palette, D2D1::ColorF& color, ID2D1SolidColorBrush* pBrush);
	void	OnCustomSlogan();
	bool	OnFontChange();
	bool	OnTextChange();
	CKD2DSizeF	GetTextBounds(CKD2DRectF& rText) const;
	void	DrawTextBounds();
	void	DrawGlyphBounds(CKD2DPointF ptBaselineOrigin, DWRITE_GLYPH_RUN const* pGlyphRun, 
		bool bTightVertBounds = false);
	void	GetRunBounds(CKD2DRectF& rRun, CKD2DPointF ptBaselineOrigin, 
		DWRITE_GLYPH_RUN const* pGlyphRun, bool bTightVertBounds = false) const;
	double	GetPhase(UINT nFlags = 0) const;
	double	GetFrameRate();
	double	GetFrameTime() const;
	bool	CreateEraser(CKD2DSizeF& szMask, FLOAT fAlpha = 1.0f);
	void	EraseBackground(D2D1_POINT_2F ptTargetOffset, D2D1_RECT_F *pImageRect = NULL);
	bool	PushDrawCommand(const CRenderCmd& cmd);
	void	UpdateCurrentSloganText();

// Regression test
	bool	SetCapture(bool bEnable = true);
	bool	CaptureFrame();
	void	RegressionTestSetup();
	bool	RegressionTest();

// Transitions, defined in separate .cpp file
	void	TransScroll();
	void	TransReveal();
	void	TransFade();
	void	TransTypewriter();
	void	TransScale();
	void	InitTiling(const CKD2DRectF& rText);
	void	TransRandTile();
	bool	TransConverge();
	bool	TransConvergeHorz(CKD2DPointF ptBaselineOrigin, DWRITE_MEASURING_MODE measuringMode, 
		DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, DWRITE_GLYPH_RUN const* pGlyphRun);
	bool	TransConvergeVert(CKD2DPointF ptBaselineOrigin, DWRITE_MEASURING_MODE measuringMode, 
		DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, DWRITE_GLYPH_RUN const* pGlyphRun);
	bool	GetLineMetrics();
	bool	MakeCharToLineTable();
	bool	TransMelt();
	bool	TransMelt(CKD2DPointF ptBaselineOrigin, DWRITE_MEASURING_MODE measuringMode, 
		DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, DWRITE_GLYPH_RUN const* pGlyphRun);
	bool	LaunchMeltWorker(int iSelSlogan = -1);
	bool	MeasureMeltStroke();
	bool	TransElevator();
	bool	TransElevator(CKD2DPointF ptBaselineOrigin, DWRITE_MEASURING_MODE measuringMode, 
		DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, DWRITE_GLYPH_RUN const* pGlyphRun);
	bool	TransClock();
	bool	TransClock(CKD2DPointF ptBaselineOrigin, DWRITE_MEASURING_MODE measuringMode, 
		DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, DWRITE_GLYPH_RUN const* pGlyphRun);
	bool	TransSkew();
	bool	TransSkew(CKD2DPointF ptBaselineOrigin, DWRITE_MEASURING_MODE measuringMode, 
		DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, DWRITE_GLYPH_RUN const* pGlyphRun);
	bool	TransExplode();
	bool	TransExplode(CKD2DPointF ptBaselineOrigin, DWRITE_MEASURING_MODE measuringMode, 
		DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, DWRITE_GLYPH_RUN const* pGlyphRun);
	bool	TransSubmarine();
	bool	TransSubmarine(CKD2DPointF ptBaselineOrigin, DWRITE_MEASURING_MODE measuringMode, 
		DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, DWRITE_GLYPH_RUN const* pGlyphRun);
	bool	TransTumble();
	bool	TransTumble(CKD2DPointF ptBaselineOrigin, DWRITE_MEASURING_MODE measuringMode, 
		DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, DWRITE_GLYPH_RUN const* pGlyphRun);
	bool	TransIris();
	bool	TransIris(CKD2DPointF ptBaselineOrigin, DWRITE_MEASURING_MODE measuringMode, 
		DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, DWRITE_GLYPH_RUN const* pGlyphRun);
};

inline bool CSloganDraw::IsTransOut() const
{
	return m_iState == ST_TRANS_OUT || m_iState == ST_PAUSE;
}

inline bool CSloganDraw::IsIdle() const
{
	return m_iState == ST_HOLD || m_iState == ST_PAUSE;
}

inline bool CSloganDraw::IsFullScreen() const
{
	return m_bIsFullScreen;
}

inline const CSloganParams& CSloganDraw::GetParams() const
{
	return *this;	// automatic upcast
}
