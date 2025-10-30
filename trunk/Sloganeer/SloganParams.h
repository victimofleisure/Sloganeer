// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      30oct25	initial version

*/

#pragma once

class CSloganParams {
public:
// Construction
	CSloganParams();

// Public data
	CStringArrayEx	m_aSlogan;	// array of slogans to display
	bool	m_bStartFullScreen;	// true if starting in full screen mode
	bool	m_bSeqSlogans;		// true if showing slogans sequentially
	int		m_nHoldDuration;	// hold duration in milliseconds
	int		m_nPauseDuration;	// pause duration in milliseconds
	float	m_fTransDuration;	// transition duration in seconds
	CString	m_sFontName;		// font name
	float	m_fFontSize;		// font size, in points
	int		m_nFontWeight;		// font weight, from 1 to 999
	D2D1::ColorF	m_clrBkgnd;	// background color
	D2D1::ColorF	m_clrDraw;	// drawing color

// Attributes
	void	SetSlogans(const LPCTSTR *aSlogan, int nSlogans);
};

class CParamsParser : public CCommandLineInfo {
public:
// Construction
	CParamsParser(CSloganParams& params);
	virtual ~CParamsParser();

// Operations
	bool	Parse();

protected:
// Constants
	enum {
		#define PARAMDEF(name) FLAG_##name,
		#include "ParamDef.h"
		FLAGS
	};
	static const LPCTSTR m_aFlag[FLAGS];

// Overrides
	virtual void ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast);

// Data members
	CSloganParams&	m_params;	// reference to parameters instance
	int		m_iFlag;	// index of flag expecting a parameter
	bool	m_bError;	// true if error occurred

// Helpers
	void	OnError(int nErrID, LPCTSTR pszParam);
};
