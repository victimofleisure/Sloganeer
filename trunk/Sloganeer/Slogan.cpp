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

#include "stdafx.h"
#include "SloganParams.h"
#include <sstream>	// for Format method

const LPCTSTR CSlogan::m_aColumnName[COLUMNS] = {
	#define SLOGANDEF(name, member) _T(#name),
	#include "ParamDef.h"	// generate code
};

const LPCTSTR CSlogan::m_aTransTypeCode[TRANS_TYPES] = {
	#define TRANSTYPEDEF(code, name) _T(#code),
	#include "ParamDef.h"	// generate code
};

CSlogan::CSlogan() :
	m_clrBkgnd(0),
	m_clrDraw(0)
{
	m_fFontSize = INVALID;
	m_nFontWeight = INVALID;
	m_nHoldDuration = INVALID;
	m_clrBkgnd.r = INVALID;
	m_clrDraw.r = INVALID;
	m_nPauseDuration = INVALID;
	m_fInTransDuration = INVALID;
	m_fOutTransDuration = INVALID;
	m_aTransType[TD_INCOMING] = INVALID;
	m_aTransType[TD_OUTGOING] = INVALID;
}

bool CSlogan::IsSameFont(const CSlogan& slogan) const
{
	// true if font name, size and weight match
	static const float fFontSizeEpsilon = 1e-4f;
	return !_tcsicmp(slogan.m_sFontName, m_sFontName)	// case insensitive comparison
		&& fabs(slogan.m_fFontSize - m_fFontSize) < fFontSizeEpsilon
		&& slogan.m_nFontWeight == m_nFontWeight;
}

bool CSlogan::IsSameColor(const D2D1::ColorF& clr1, const D2D1::ColorF& clr2)
{
	// true if colors match exactly; intentional binary compare
	return !memcmp(&clr1, &clr2, sizeof(D2D1::ColorF));
}

int CSlogan::FindColumnName(LPCTSTR pszName)
{
	for (int iCol = 0; iCol < COLUMNS; iCol++) {	// for each column
		if (!_tcsicmp(m_aColumnName[iCol], pszName))	// if names match; case insensitive
			return iCol;	// return column index
	}
	return -1;	// name not found
}

int CSlogan::FindTransTypeCode(LPCTSTR pszCode)
{
	for (int iType = 0; iType < TRANS_TYPES; iType++) {	// for each transition type
		if (!_tcsicmp(m_aTransTypeCode[iType], pszCode))	// if codes match; case insensitive
			return iType;	// return transition type index
	}
	return -1;	// code not found
}

void CSlogan::Customize(const CSlogan& slogan)
{
	// for each member, override if it contains a valid value
	#define SLOGANDEF(name, member) if (IsValid(slogan.m_##member)) \
		m_##member = slogan.m_##member;
	#include "ParamDef.h"	// generate code
}

template<class CharT, class Traits>	// use template to handle TCHAR
static std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const D2D1::ColorF& c)
{
	os << c.r << ' ' << c.g << ' ' << c.b << ' ' << c.a;
	return os;
}

template<class CharT, class Traits>	// use template to handle TCHAR
static std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const CString& s)
{
	os << s.GetString();
	return os;
}

CString CSlogan::Format() const
{
	std::basic_stringstream<TCHAR> ss;
	#define SLOGANDEF(name, member) ss << m_##member << ',';
	#include "ParamDef.h"	// generate code
	CString	s(ss.str().c_str());
	s.Delete(s.GetLength() - 1);	// remove trailing separator
	return s;
}

void CSloganArray::DumpSlogans() const
{
	int	nSlogans = GetSize();
	for (int iSlogan = 0; iSlogan < nSlogans; iSlogan++) {
		const CSlogan& slogan = GetAt(iSlogan);
		_tprintf(_T("[%s]\n"), slogan.Format().GetString());
	}
}
