// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      18nov25	initial version

*/

#pragma once

class CSlogan {
public:
// Construction
	CSlogan();

// Constants
	enum {	// columns
		#define SLOGANDEF(name, member) COL_##name,
		#include "ParamDef.h"	// generate code
		COLUMNS
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

// Public data
	CString	m_sText;			// text to display
	CString	m_sFontName;		// font name
	float	m_fFontSize;		// font size, in points
	int		m_nFontWeight;		// font weight, from 1 to 999
	int		m_nHoldDuration;	// hold duration in milliseconds
	int		m_nPauseDuration;	// pause duration in milliseconds
	float	m_fInTransDuration;	// incoming transition duration in seconds
	float	m_fOutTransDuration;	// outgoing transition duration in seconds
	D2D1::ColorF	m_clrBkgnd;	// background color
	D2D1::ColorF	m_clrDraw;	// drawing color
	int		m_aTransType[TRANS_DIRS];	// transition type for each direction

// Attributes
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
	void	Customize(const CSlogan& slogan);

protected:
	static const LPCTSTR m_aColumnName[COLUMNS];	// column names
	static const LPCTSTR m_aTransTypeCode[TRANS_TYPES];	// transition type codes
};

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

class CSloganArray : public CArrayEx<CSlogan, CSlogan&> {
public:
	void	DumpSlogans() const;
};
