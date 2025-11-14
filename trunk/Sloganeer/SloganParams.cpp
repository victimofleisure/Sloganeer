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

*/

#include "stdafx.h"
#include "Sloganeer.h"
#include "SloganParams.h"
#include "StdioFileEx.h"
#include <string>	// for stoi and stof

const LPCTSTR CParamsParser::m_aFlag[FLAGS] = {
	#define PARAMDEF(name) _T(#name),
	#include "ParamDef.h"
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

CParamsParser::~CParamsParser()
{
}

void CSloganParams::SetSlogans(const LPCTSTR *aSlogan, int nSlogans)
{
	m_aSlogan.SetSize(nSlogans);	// allocate slogan array
	for (int iSlogan = 0; iSlogan < nSlogans; iSlogan++) {	// for each slogan
		m_aSlogan[iSlogan] = aSlogan[iSlogan];	// copy string to array element
	}
}

CParamsParser::CParamsParser(CSloganParams& params) : m_params(params)
{
	m_iFlag = -1;
	m_bError = false;
	m_bHasOutDur = false;
	m_bHasRandSeed = false;
}

void CParamsParser::OnError(int nErrID, LPCTSTR pszParam)
{
	CString	sErrMsg;
	AfxFormatString1(sErrMsg, nErrID, pszParam);
	AfxMessageBox(sErrMsg);
	m_bError = true;
}

// define a conversion function template specialization for the specified type
#define CONVERSION_FUNC_DEF(type, function) \
	template<> void CParamsParser::Convert(LPCTSTR pszParam, type& val) { \
		val = std::function(pszParam); \
	}

// note that stoi and stof throw out_of_range exceptions
CONVERSION_FUNC_DEF(int, stoi);
CONVERSION_FUNC_DEF(UINT, stoi);
CONVERSION_FUNC_DEF(LONG, stoi);
CONVERSION_FUNC_DEF(float, stof);

template<typename T> bool CParamsParser::Scan(LPCTSTR pszParam, T& val, T minVal, T maxVal)
{
	T param;
	Convert(pszParam, param);
	// if range is specified and parameter is outside of range
	if (minVal != maxVal && (param < minVal || param > maxVal)) {
		OnError(IDS_ERR_PARAM_RANGE, m_aFlag[m_iFlag]);
		return false;
	}
	val = param;
	return true;
}

template<> bool CParamsParser::Scan(LPCTSTR pszParam, CSize& val, CSize minVal, CSize maxVal)
{
	LPCTSTR	pszSep = _tcschr(pszParam, 'x');
	if (pszSep == NULL) {
		OnError(IDS_ERR_PARAM_INVALID, m_aFlag[m_iFlag]);
		return false;
	}
	CSize	sz;
	if (!Scan(pszParam, sz.cx, minVal.cx, minVal.cx))
		return false;
	if (!Scan(pszSep + 1, sz.cy, minVal.cy, minVal.cy))
		return false;
	val = sz;
	return true;
}

void CParamsParser::ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast)
{
	if (bFlag) {	// if flag
		if (m_iFlag >= 0) {	// if parameter expected
			OnError(IDS_ERR_PARAM_MISSING, m_aFlag[m_iFlag]);
			m_iFlag = -1;
		}
		int iFlag;
		for (iFlag = 0; iFlag < FLAGS; iFlag++) {	// for each supported flag
			if (!_tcsicmp(pszParam, m_aFlag[iFlag]))	// if flag matches
				break;	// flag found
		}
		if (iFlag < FLAGS) {	// if flag found
			switch (iFlag) {
			case FLAG_fullscreen:
				m_params.m_bStartFullScreen = true;
				break;
			case FLAG_seqtext:
				m_params.m_bSeqSlogans = true;	// sequential order
				break;
			case FLAG_nowrap:
				m_params.m_bNoWordWrap = true;	// disable word wrapping
				break;
			default:
				m_iFlag = iFlag;	// flag requires a parameter
			}
		} else {	// flag not found
			OnError(IDS_ERR_UNKNOWN_FLAG, pszParam);
		}
	} else {	// parameter
		float	fParam;
		if (m_iFlag >= 0) {	// if parameter expected
			try {
				switch (m_iFlag) {
				case FLAG_fontsize:
					Scan(pszParam, m_params.m_fFontSize);
					break;
				case FLAG_fontname:
					m_params.m_sFontName = pszParam;
					break;
				case FLAG_fontweight:
					Scan(pszParam, m_params.m_nFontWeight, 1, 999);
					break;
				case FLAG_transdur:
					Scan(pszParam, m_params.m_fInTransDuration);
					break;
				case FLAG_outdur:
					Scan(pszParam, m_params.m_fOutTransDuration);
					m_bHasOutDur = true;
					break;
				case FLAG_holddur:
					if (Scan(pszParam, fParam))
						m_params.m_nHoldDuration = Round(fParam * 1000);	// convert to milliseconds
					break;
				case FLAG_pausedur:
					if (Scan(pszParam, fParam))
						m_params.m_nPauseDuration = Round(fParam * 1000);	// convert to milliseconds
					break;
				case FLAG_bgcolor:
					m_params.m_clrBkgnd = D2D1::ColorF(std::stoi(pszParam, 0, 16));	// hexadecimal
					break;
				case FLAG_drawcolor:
					m_params.m_clrDraw = D2D1::ColorF(std::stoi(pszParam, 0, 16));	// hexadecimal
					break;
				case FLAG_seed:
					Scan(pszParam, m_params.m_nRandSeed);
					m_bHasRandSeed = true;
					break;
				case FLAG_easing:
					Scan(pszParam, fParam);
					m_params.m_fEasing = fParam / 100;	// convert percentage to fraction
					break;
				case FLAG_record:
					m_params.m_sRecFolderPath = pszParam;
					break;
				case FLAG_recsize:
					Scan(pszParam, m_params.m_szRecFrameSize, CSize(1, 1), CSize(INT_MAX, INT_MAX));
					break;
				case FLAG_recrate:
					Scan(pszParam, m_params.m_fRecFrameRate, 1e-3f, 1e3f);	// avoid divide by zero
					break;
				case FLAG_recdur:
					Scan(pszParam, m_params.m_fRecDuration);
					break;
				default:
					NODEFAULTCASE;	// logic error
				}
			} catch (std::exception) {
				OnError(IDS_ERR_PARAM_INVALID, m_aFlag[m_iFlag]);
			}
			m_iFlag = -1;	// mark parameter consumed
		} else {	// parameter not expected
			m_strFileName = pszParam;	// assume filename
		}
	}
	if (bLast) {	// if last token
		if (m_iFlag >= 0) {	// if parameter expected
			OnError(IDS_ERR_PARAM_MISSING, m_aFlag[m_iFlag]);
		}
		if (!m_bHasOutDur)	// if outgoing transition duration not specified
			m_params.m_fOutTransDuration = m_params.m_fInTransDuration;	// same as incoming
		if (!m_bHasRandSeed)	// if random number seed not specified
			m_params.m_nRandSeed = GetTickCount();
	}
}

bool CParamsParser::Parse()
{
	theApp.ParseCommandLine(*this);
	if (!m_strFileName.IsEmpty()) {	// if file name specified
		// assume UTF-8 text file containing slogans to display, one per line
		CStdioFileEx	fSlogan(m_strFileName, CFile::modeRead);
		CStringArrayEx	aSlogan;
		CString	sSlogan;
		while (fSlogan.ReadString(sSlogan)) {	// read slogan string
			if (!sSlogan.IsEmpty()) {	// if string isn't empty
				sSlogan.Replace('\t', '\n');	// replace tabs with newlines
				aSlogan.Add(sSlogan);	// add slogan to array
			}
		}
		m_params.m_aSlogan = aSlogan;	// pass slogans to drawing object
	} else {	// no slogans specified
		m_params.m_aSlogan.Add(_T("YOUR TEXT"));	// add placeholder
	}
	return !m_bError;	// return true if no errors
}
