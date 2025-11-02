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
}

void CParamsParser::OnError(int nErrID, LPCTSTR pszParam)
{
	CString	sErrMsg;
	AfxFormatString1(sErrMsg, nErrID, pszParam);
	AfxMessageBox(sErrMsg);
	m_bError = true;
}

void CParamsParser::ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast)
{
	if (bFlag) {	// if flag
		if (m_iFlag >= 0) {	// if parameter expected
			OnError(IDS_ERR_MISSING_PARAM, m_aFlag[m_iFlag]);
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
		if (m_iFlag >= 0) {	// if parameter expected
			switch (m_iFlag) {
			case FLAG_fontsize:
				m_params.m_fFontSize = std::stof(pszParam);
				break;
			case FLAG_fontname:
				m_params.m_sFontName = pszParam;
				break;
			case FLAG_fontweight:
				m_params.m_nFontWeight = std::stoi(pszParam);
				break;
			case FLAG_transdur:
				m_params.m_fInTransDuration = std::stof(pszParam);
				break;
			case FLAG_outdur:
				m_params.m_fOutTransDuration = std::stof(pszParam);
				m_bHasOutDur = true;
				break;
			case FLAG_holddur:
				m_params.m_nHoldDuration = Round(std::stof(pszParam) * 1000);
				break;
			case FLAG_pausedur:
				m_params.m_nPauseDuration = Round(std::stof(pszParam) * 1000);
				break;
			case FLAG_bgcolor:
				m_params.m_clrBkgnd = D2D1::ColorF(std::stoi(pszParam, 0, 16));	// hexadecimal
				break;
			case FLAG_drawcolor:
				m_params.m_clrDraw = D2D1::ColorF(std::stoi(pszParam, 0, 16));	// hexadecimal
				break;
			default:
				NODEFAULTCASE;	// logic error
			}
			m_iFlag = -1;	// mark parameter consumed
		} else {	// parameter not expected
			m_strFileName = pszParam;	// assume filename
		}
	}
	if (bLast) {	// if last token
		if (m_iFlag >= 0) {	// if parameter expected
			OnError(IDS_ERR_MISSING_PARAM, m_aFlag[m_iFlag]);
		}
		if (!m_bHasOutDur)	// if outgoing transition duration not specified
			m_params.m_fOutTransDuration = m_params.m_fInTransDuration;	// same as incoming
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
