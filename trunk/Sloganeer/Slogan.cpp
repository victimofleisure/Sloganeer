// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      18nov25	initial version
		01		11dec25	add update columns

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

const TCHAR CSlogan::m_cEsc = '\\';	// escape character

const D2D1::ColorF CSlogan::INVALID_COLOR(INVALID, INVALID, INVALID, INVALID);

CSlogan::CSlogan() :
	m_clrBkgnd(INVALID_COLOR),
	m_clrDraw(INVALID_COLOR)
{
	InitSloganColumns();
}

CSlogan::CSlogan(bool bAppDefaults) :
	m_clrBkgnd(D2D1::ColorF::Black),
	m_clrDraw(D2D1::ColorF::White)
{
	UNREFERENCED_PARAMETER(bAppDefaults);	// argument is for ctor discrimination only
	InitSloganColumns();
	m_sFontName = L"Arial";
	m_fFontSize = 150.0f;
	m_nFontWeight = DWRITE_FONT_WEIGHT_BLACK;
	m_fHoldDur = 1.0f;
	m_fPauseDur = 0;
	m_fInTransDur = 2.0f;
	m_fOutTransDur = 2.0f;
}

void CSlogan::InitSloganColumns()
{
	m_fFontSize = INVALID;
	m_nFontWeight = INVALID;
	m_fHoldDur = INVALID;
	m_fPauseDur = INVALID;
	m_fInTransDur = INVALID;
	m_fOutTransDur = INVALID;
	m_aTransType[TD_INCOMING] = INVALID;
	m_aTransType[TD_OUTGOING] = INVALID;
	m_nCustomColMask = 0;
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

void CSlogan::UpdateColumns(const CSlogan& src, UINT nColMask)
{
	// update uncustomized columns that are included in nColMask
	nColMask &= ~m_nCustomColMask;	// exclude customized columns
	#define SLOGANDEF(name, member) if (nColMask & (1 << COL_##name)) \
		m_##member = src.m_##member;
	#include "ParamDef.h"	// generate code
}

bool CSlogan::EscapeChars(CString& sText)
{
	CString	sEdit(sText);	// copy text argument to edit buffer
	int	iPos = 0;
	// while escape character is found in edit buffer
	while ((iPos = sEdit.Find(m_cEsc, iPos)) >= 0) {
		sEdit.Delete(iPos);	// delete escape char
		if (iPos < sEdit.GetLength()) {	// if not end of string
			TCHAR	cIn = sEdit[iPos];	// copy escape argument
			TCHAR	cOut = 0;
			sEdit.Delete(iPos);	// delete escape argument
			switch (cIn) {	// switch on escape argument
			case 'n':
				cOut = '\n';	// newline
				break;
			case 't':
				cOut = '\t';	// tab
				break;
			case m_cEsc:
				cOut = m_cEsc;	// escape
				break;
			default:
				return false;	// unknown escape sequence
			}
			sEdit.Insert(iPos, cOut);	// insert replacement char
			iPos++;	// bump char position
		}
	}
	sText = sEdit;	// pass edited string to caller
	return true;
}

void CSlogan::UnescapeChars(CString& sText)
{
	int	iPos = 0;
	// while characters remain unprocessed
	while (iPos < sText.GetLength()) {
		TCHAR	cIn = sText[iPos];
		TCHAR	cOut = 0;
		switch (cIn) {
		case '\n':
			cOut = 'n';	// newline
			break;
		case '\t':
			cOut = 't';	// tab
			break;
		case m_cEsc:
			cOut = m_cEsc;	// escape
			break;
		}
		if (cOut) {	// if char needs escaping
			sText.Delete(iPos);
			sText.Insert(iPos, m_cEsc);
			iPos++;
			sText.Insert(iPos, cOut);
		}
		iPos++;
	}
}

void CSloganArray::DumpSlogans() const
{
	int	nSlogans = GetSize();
	for (int iSlogan = 0; iSlogan < nSlogans; iSlogan++) {
		_tprintf(_T("[%s]\n"), GetAt(iSlogan).Format().GetString());
	}
}

void CSloganArray::UpdateColumns(const CSlogan& src, UINT nColMask)
{
	int	nSlogans = GetSize();
	for (int iSlogan = 0; iSlogan < nSlogans; iSlogan++) {
		GetAt(iSlogan).UpdateColumns(src, nColMask);
	}
}
