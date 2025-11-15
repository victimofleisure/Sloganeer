// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      30oct25	initial version
        01      12nov25	add easing
		02		14nov25	add recording
		03		15nov25	add color names

*/

#include "stdafx.h"
#include "Sloganeer.h"
#include "SloganParams.h"
#include "StdioFileEx.h"

const LPCTSTR CSloganParams::m_aColorName[] = {
	#define COLORNAMEDEF(name) _T(#name),
	#include "ColorNameDef.h"
};

const COLORREF CSloganParams::m_aColorVal[] = {
	#define COLORNAMEDEF(name) D2D1::ColorF::##name,
	#include "ColorNameDef.h"
};

CSloganParams::CSloganParams() :
	m_clrBkgnd(D2D1::ColorF::Black),
	m_clrDraw(D2D1::ColorF::White)
{
	m_bStartFullScreen = false;
	m_bSeqSlogans = false;
	m_bNoWordWrap = false;
	m_nHoldDuration = 1000;
	m_nPauseDuration = 0;
	m_fInTransDuration = 2.0f;
	m_fOutTransDuration = 2.0f;
	m_sFontName = L"Arial";
	m_fFontSize = 150.0f;
	m_nFontWeight = DWRITE_FONT_WEIGHT_BLACK;
	m_nRandSeed = 0;
	m_fEasing = 0.15f;
	m_szRecFrameSize = CSize(1920, 1080);
	m_fRecFrameRate = 60;
	m_fRecDuration = 60;
}

void CSloganParams::ReadSlogans(LPCTSTR pszPath)
{
	// assume UTF-8 text file containing slogans to display, one per line
	CStdioFileEx	fSlogan(pszPath, CFile::modeRead);
	CStringArrayEx	aSlogan;
	CString	sSlogan;
	while (fSlogan.ReadString(sSlogan)) {	// read slogan string
		if (!sSlogan.IsEmpty()) {	// if string isn't empty
			sSlogan.Replace('\t', '\n');	// replace tabs with newlines
			aSlogan.Add(sSlogan);	// add slogan to array
		}
	}
	m_aSlogan = aSlogan;	// pass slogans to drawing object
}

void CSloganParams::SetSlogans(const LPCTSTR *aSlogan, int nSlogans)
{
	m_aSlogan.SetSize(nSlogans);	// allocate slogan array
	for (int iSlogan = 0; iSlogan < nSlogans; iSlogan++) {	// for each slogan
		m_aSlogan[iSlogan] = aSlogan[iSlogan];	// copy string to array element
	}
}

W64INT CSloganParams::BinarySearch(const LPCTSTR* aStr, W64INT nStrings, LPCTSTR pszTarget)
{
	W64INT iStart = 0;
	W64INT iEnd = nStrings - 1;
	while (iStart <= iEnd) {
		W64INT iMid = (iStart + iEnd) / 2;
		int	nResult = _tcsicmp(pszTarget, aStr[iMid]);
		if (!nResult)
			return iMid;
		if (nResult < 0)
			iEnd = iMid - 1;
		else
			iStart = iMid + 1;
	}
	return -1;
}

COLORREF CSloganParams::FindColor(LPCTSTR pszName)
{
	CString	sName(pszName);
	sName.MakeLower();
	sName.Replace(_T("grey"), _T("gray"));	// allow British spelling
	W64INT iColor = BinarySearch(m_aColorName, _countof(m_aColorName), sName);
	if (iColor >= 0)	// if color name found
		return m_aColorVal[iColor];	// return color value
	return UINT_MAX;	// return error
}
