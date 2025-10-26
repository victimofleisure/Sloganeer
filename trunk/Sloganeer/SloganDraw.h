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

#pragma once

#include "RenderThread.h"
#include "Event.h"
#include "RandList.h"
#include "Benchmark.h"

class CSloganDraw : public CRenderThread {
public:
// Construction
	CSloganDraw();
	~CSloganDraw();
	bool	Create(HWND hWnd);
	void	Destroy();

// Attributes
	bool	IsFullScreen() const;
	bool	HasSlogans() const;
	void	SetSlogans(const CStringArrayEx& aSlogan);
	void	SetSlogans(const LPCTSTR *aSlogan, int nSlogans);
	void	SetFontSize(float fSize);
	void	SetFontName(CString sName);
	void	SetFontWeight(int nWeight);
	void	SetTransDuration(float fSeconds);
	void	SetHoldDuration(float fSeconds);

// Operations
	void	Resize();
	bool	FullScreen(bool bEnable);

protected:
// Constants
	enum {	// display states
		ST_TRANS_IN,	// transition in
		ST_HOLD,		// freeze frame
		ST_TRANS_OUT,	// transition out
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
		TRANS_TYPES
	};
	enum {
		AA_MARGIN = 1,	// extra margin to account for antialiasing
	};
	const D2D1::ColorF	m_clrBkgnd;	// background color
	const D2D1::ColorF	m_clrDraw;	// drawing color

// Members
	CComPtr<ID2D1SolidColorBrush>	m_pBkgndBrush;	// background brush interface
	CComPtr<ID2D1SolidColorBrush>	m_pDrawBrush;	// drawing brush interface
	CComPtr<ID2D1SolidColorBrush>	m_pVarBrush;	// variable brush interface
	CComPtr<IDWriteFactory>	m_pDWriteFactory;	// DirectWrite factory interface
	CComPtr<IDWriteTextFormat>	m_pTextFormat;	// text format interface
	CComPtr<IDWriteTextLayout>	m_pTextLayout;	// text layout interface
	DWRITE_TEXT_METRICS	m_textMetrics;	// text metrics
	DWRITE_OVERHANG_METRICS	m_overhangMetrics;	// overhang metrics
	CStringArrayEx	m_aSlogan;	// array of slogans to display
	WEvent	m_evtWake;			// during hold, signals wake from idle
	CRandList	m_rlTransType;	// randomized list of transition types
	CRandList	m_rlSloganIdx;	// randomized list of slogan indices
	CBenchmark	m_timerTrans;	// high-performance transition timer
	ULONGLONG	m_nWakeTime;	// during hold, wake time in CPU ticks
	double	m_fTransProgress;	// transition progress, normalized from 0 to 1
	bool	m_bThreadExit;		// true if render thread exit requested
	bool	m_bIsFullScreen;	// true if in full screen mode
	int		m_iState;			// index of current state
	int		m_iSlogan;			// index of current slogan
	int		m_iTransType;		// index of current transition type
	int		m_nHoldDuration;	// hold duration in milliseconds
	float	m_fTransDuration;	// transition duration in seconds
	float	m_fFontSize;		// font size, in points
	CString	m_sFontName;		// font name
	int		m_nFontWeight;		// font weight, from 1 to 999

// Overrides
	virtual void	OnError(HRESULT hr, LPCSTR pszSrcFileName, int nLineNum, LPCSTR pszSrcFileDate);
	virtual	bool	CreateUserResources();
	virtual	void	DestroyUserResources();
	virtual	bool	OnThreadCreate();
	virtual	void	OnResize();
	virtual	bool	OnDraw();

// Helpers
	void	StartCycle();
	void	StartHold();
	void	ContinueHold();
	bool	OnFontChange();
	bool	OnTextChange();
	void	TransScroll();
	void	TransReveal();
	void	TransFade();
	void	TransTypewriter();
	void	TransScale();
};

inline bool CSloganDraw::IsFullScreen() const
{
	return m_bIsFullScreen;
}

inline bool CSloganDraw::HasSlogans() const
{
	return !m_aSlogan.IsEmpty();
}

inline void CSloganDraw::SetFontSize(float fSize)
{
	m_fFontSize = fSize;
}

inline void CSloganDraw::SetFontName(CString sName)
{
	m_sFontName = sName;
}

inline void CSloganDraw::SetFontWeight(int nWeight)
{
	m_nFontWeight = nWeight;
}

inline void CSloganDraw::SetTransDuration(float fSeconds)
{
	m_fTransDuration = fSeconds;
}

inline void CSloganDraw::SetHoldDuration(float fSeconds)
{
	m_nHoldDuration = Round(fSeconds * 1000);	// convert to millseconds
}
