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
		04		18nov25	add CSV support
        05      25nov25	add color palette and cycling
		06		03dec25	handle escape sequences in text file
		07		03dec25	add pipe name attribute

*/

#include "stdafx.h"
#include "Sloganeer.h"
#include "SloganParams.h"
#include "StdioFileEx.h"
#include "ParamParser.h"

#define DTF(x) static_cast<float>(x)

const LPCTSTR CSloganParams::m_aColorName[] = {
	#define COLORNAMEDEF(name) _T(#name),
	#include "ColorNameDef.h"	// generate code
};

const COLORREF CSloganParams::m_aColorVal[] = {
	#define COLORNAMEDEF(name) D2D1::ColorF::##name,
	#include "ColorNameDef.h"	// generate code
};

CSloganParams::CSloganParams() : CSlogan(true)	// app defaults
{
	m_bStartFullScreen = false;
	m_bSeqSlogans = false;
	m_bNoWordWrap = false;
	m_bCustomSlogans = false;
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
	CSloganArray	aSlogan;
	CString	sText;
	CSlogan	slogan(*this);	// set slogan attributes to defaults
	while (fSlogan.ReadString(sText)) {	// read slogan text
		if (!sText.IsEmpty()) {	// if string isn't empty
			EscapeChars(sText);	// // handle escape sequences
			slogan.m_sText = sText;
			aSlogan.Add(slogan);	// add slogan to array
		}
	}
	m_aSlogan = aSlogan;	// pass slogans to drawing object
}

void CSloganParams::SetSlogans(const LPCTSTR *aSlogan, int nSlogans)
{
	m_aSlogan.SetSize(nSlogans);	// allocate slogan array
	for (int iSlogan = 0; iSlogan < nSlogans; iSlogan++) {	// for each slogan
		m_aSlogan[iSlogan].m_sText = aSlogan[iSlogan];	// copy string to array element
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

CSloganParams::CPalette::CPalette()
{
	m_fCycleFreq = 1;	// so cycling is visibly on if user forgets to specify frequency
}

void CSloganParams::CPalette::Read(LPCTSTR pszPath)
{
	FastRemoveAll();	// in case of reuse
	CStdioFileEx	fPalette(pszPath, CFile::modeRead);
	CString	sLine;
	D2D1::ColorF	color(0);
	while (fPalette.ReadString(sLine)) {	// for each line
		if (!sLine.IsEmpty()) {	// if line not empty
			// convert string to a color; error throws std::exception
			CParamParser::ScanColor(sLine, color);
			Add(color);	// add color to array
		}
	}
}

void CSloganParams::CPalette::CycleColor(double fElapsedTime, D2D1::ColorF& color) const
{
	// map elapsed time and cycle frequency onto a circular palette,
	// interpolating between adjacent entries
	ASSERT(m_fCycleFreq >= 0);	// negative frequency is unsupported
	int	nColors = GetSize();
	if (nColors <= 0)	// if empty palette
		return;	// nothing to do
	// phase within the cycle, in [0,1]
	double	fCyclePhase = fElapsedTime * m_fCycleFreq;
	fCyclePhase -= floor(fCyclePhase);
	// position along palette ring, in [0, nColors]
	double	fPalPos = fCyclePhase * nColors;
	int	iColor0 = Trunc(fPalPos);
	if (iColor0 >= nColors)	// if color index out of range
		iColor0 = 0;	// wrap around
	int	iColor1 = iColor0 + 1;
	if (iColor1 >= nColors)	// if color index out of range
		iColor1 = 0;	// wrap around
	// local phase between these two colors, in [0,1]
	double	fBlend = fPalPos - floor(fPalPos);
	const D2D1_COLOR_F&	clr0 = GetAt(iColor0);
	const D2D1_COLOR_F&	clr1 = GetAt(iColor1);
	color = D2D1::ColorF(
		DTF(Lerp(clr0.r, clr1.r, fBlend)),
		DTF(Lerp(clr0.g, clr1.g, fBlend)),
		DTF(Lerp(clr0.b, clr1.b, fBlend)),
		DTF(Lerp(clr0.a, clr1.a, fBlend)));
}

bool CSloganParams::IsPipeName(CString sPath)
{
	// return true if path is a pipe name
	return !sPath.Left(9).CompareNoCase(_T("\\\\.\\pipe\\"));
}
