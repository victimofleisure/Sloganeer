// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      18nov25	initial version
        01      25nov25	add customized column mask
		02		11dec25	add update columns
		03		21dec25	refactor hold and pause

*/

#pragma once

class CSlogan {
public:
// Construction
	CSlogan();
	CSlogan(bool bAppDefaults);

// Constants
	enum {	// columns
		#define SLOGANDEF(name, member) COL_##name,
		#include "ParamDef.h"	// generate code
		COLUMNS
	};
	enum {	// per-column bitmasks
		#define SLOGANDEF(name, member) CBM_##name = 1 << COL_##name,
		#include "ParamDef.h"	// generate code
		// column group bitmasks
		CGBM_FONT = CBM_fontname | CBM_fontsize | CBM_fontwt,
		CGBM_COLOR = CBM_bgclr | CBM_drawclr,
		CGBM_DURATION = CBM_transdur | CBM_holddur | CBM_outdur | CBM_pausedur,
		CGBM_TRANSITION = CBM_intrans | CBM_outtrans,
	};
	enum {	// transition types
		#define TRANSTYPEDEF(code, name) TT_##name,
		#include "ParamDef.h"	// generate code
		TRANS_TYPES
	};
	enum {	// transition directions
		TD_INCOMING,
		TD_OUTGOING,
		TRANS_DIRS,
	};
	enum {
		INVALID = -1,
	};
	static const D2D1::ColorF INVALID_COLOR;

// Public data
	CString	m_sText;			// text to display
	CString	m_sFontName;		// font name
	float	m_fFontSize;		// font size, in DIPs
	int		m_nFontWeight;		// font weight, from 1 to 999
	float	m_fHoldDur;			// hold duration in seconds
	float	m_fPauseDur;		// pause duration in seconds
	float	m_fInTransDur;		// incoming transition duration in seconds
	float	m_fOutTransDur;		// outgoing transition duration in seconds
	D2D1::ColorF	m_clrBkgnd;	// background color
	D2D1::ColorF	m_clrDraw;	// drawing color
	int		m_aTransType[TRANS_DIRS];	// transition type for each direction
	UINT	m_nCustomColMask;	// bitmask indicating which columns are customized

// Attributes
	bool	IsCustomized(int iCol) const;
	template<typename T> static bool IsValid(const T& val) { return val >= 0; }
	static bool IsValid(const CString& val) { return !val.IsEmpty(); }
	static bool IsValid(const D2D1::ColorF& val) { return val.r >= 0; }
	bool	IsSameFont(const CSlogan& slogan) const;
	bool	IsSameColor(const D2D1::ColorF& clr1, const D2D1::ColorF& clr2);
	static const LPCTSTR GetColumnName(int iCol);
	static const LPCTSTR GetTransTypeCode(int iTransType);
	static int	FindColumnName(LPCTSTR pszName);
	static int	FindTransTypeCode(LPCTSTR pszCode);
	CString	Format() const;

// Operations
	void	InitSloganColumns();
	void	UpdateColumns(const CSlogan& src, UINT nColMask);
	static double	Lerp(double a, double b, double t);
	static bool	EscapeChars(CString& sText);
	static void UnescapeChars(CString& sText);

protected:
	static const LPCTSTR m_aColumnName[COLUMNS];	// column names
	static const LPCTSTR m_aTransTypeCode[TRANS_TYPES];	// transition type codes
	static const TCHAR m_cEsc;	// escape character
};

inline bool CSlogan::IsCustomized(int iCol) const
{
	ASSERT(iCol >= 0 && iCol < COLUMNS);
	return (m_nCustomColMask & (1 << iCol)) != 0;
}

inline const LPCTSTR CSlogan::GetColumnName(int iCol)
{
	// validate column index
	ASSERT(iCol >= 0 && iCol < COLUMNS);
	return m_aColumnName[iCol];
}

inline const LPCTSTR CSlogan::GetTransTypeCode(int iTransType)
{
	// validate transition type index
	ASSERT(iTransType >= 0 && iTransType < TRANS_TYPES);
	return m_aTransTypeCode[iTransType];
}

inline double CSlogan::Lerp(double a, double b, double t)
{
	return t * b + (1 - t) * a;
}

class CSloganArray : public CArrayEx<CSlogan, CSlogan&> {
public:
	void	DumpSlogans() const;
	void	UpdateColumns(const CSlogan& src, UINT nColMask);
};
