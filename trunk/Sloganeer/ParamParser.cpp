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
#include "ParamParser.h"
#include <string>	// for stoi and stof
#include "VersionInfo.h"
#include "HelpDlg.h"
#include "AboutDlg.h"

const LPCTSTR CParamParser::m_aFlag[FLAGS] = {
	#define PARAMDEF(name) _T(#name),
	#include "ParamDef.h"
};

const int CParamParser::m_aFlagHelpID[FLAGS] = {
	#define PARAMDEF(name) IDS_FLAG_HELP_##name,
	#include "ParamDef.h"
};

const int CParamParser::m_aFlagExampleID[EXAMPLES] = {
	#define HELPEXAMPLEDEF(name) IDS_FLAG_EXAMPLE_##name,
	#include "ParamDef.h"
};

const LPCTSTR CParamParser::m_pszCopyrightNotice = _T("Copyleft 2025 Chris Korda");
const LPCTSTR CParamParser::m_sDefaultSlogan = _T("YOUR TEXT");

CParamParser::CParamParser()
{
	m_iFlag = -1;
	m_bError = false;
	m_bHasOutDur = false;
	m_bHasRandSeed = false;
	m_bShowHelp = false;
	m_bShowLicense = false;
}

CParamParser::~CParamParser()
{
}

void CParamParser::OnError(int nErrID, LPCTSTR pszParam)
{
	CString	sErrMsg;
	AfxFormatString1(sErrMsg, nErrID, pszParam);
	AfxMessageBox(sErrMsg);
	m_bError = true;
}

// define a conversion function template specialization for the specified type
#define CONVERSION_FUNC_DEF(type, function) \
	template<> void CParamParser::Convert(LPCTSTR pszParam, type& val) { \
		val = std::function(pszParam); \
	}

// note that stoi and stof throw out_of_range exceptions
CONVERSION_FUNC_DEF(int, stoi);
CONVERSION_FUNC_DEF(UINT, stoi);
CONVERSION_FUNC_DEF(LONG, stoi);
CONVERSION_FUNC_DEF(float, stof);

template<typename T> bool CParamParser::Scan(LPCTSTR pszParam, T& val, T minVal, T maxVal)
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

template<> bool CParamParser::Scan(LPCTSTR pszParam, CSize& val, CSize minVal, CSize maxVal)
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

bool CParamParser::Scan(LPCTSTR pszParam, D2D1::ColorF& color)
{
	COLORREF	clr = CSloganParams::FindColor(pszParam);	// find color name
	if (clr != UINT_MAX) {	// if color name found
		color = clr;
	} else {	// not a color name
		color = D2D1::ColorF(std::stoi(pszParam, 0, 16));	// hexadecimal
	}
	return true;
}

void CParamParser::ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast)
{
	// ParseCommandLine calls this callback for each parameter string
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
			case FLAG_help:
				m_bShowHelp = true;
				break;
			case FLAG_fullscreen:
				m_bStartFullScreen = true;
				break;
			case FLAG_seqtext:
				m_bSeqSlogans = true;	// sequential order
				break;
			case FLAG_nowrap:
				m_bNoWordWrap = true;	// disable word wrapping
				break;
			case FLAG_license:
				m_bShowLicense = true;
				break;
			default:
				m_iFlag = iFlag;	// flag requires a parameter
			}
		} else {	// flag not found
			if (!_tcsicmp(pszParam, _T("h"))) {	// if help alias
				m_bShowHelp = true;
			} else {	// unknown flag
				OnError(IDS_ERR_UNKNOWN_FLAG, pszParam);
			}
		}
	} else {	// parameter
		float	fParam;
		if (m_iFlag >= 0) {	// if parameter expected
			try {
				switch (m_iFlag) {
				case FLAG_fontsize:
					Scan(pszParam, m_fFontSize);
					break;
				case FLAG_fontname:
					m_sFontName = pszParam;
					break;
				case FLAG_fontweight:
					Scan(pszParam, m_nFontWeight, 1, 999);
					break;
				case FLAG_transdur:
					Scan(pszParam, m_fInTransDuration);
					break;
				case FLAG_outdur:
					Scan(pszParam, m_fOutTransDuration);
					m_bHasOutDur = true;
					break;
				case FLAG_holddur:
					if (Scan(pszParam, fParam))
						m_nHoldDuration = Round(fParam * 1000);	// convert to milliseconds
					break;
				case FLAG_pausedur:
					if (Scan(pszParam, fParam))
						m_nPauseDuration = Round(fParam * 1000);	// convert to milliseconds
					break;
				case FLAG_bgcolor:
					Scan(pszParam, m_clrBkgnd);
					break;
				case FLAG_drawcolor:
					Scan(pszParam, m_clrDraw);
					break;
				case FLAG_seed:
					Scan(pszParam, m_nRandSeed);
					m_bHasRandSeed = true;
					break;
				case FLAG_easing:
					Scan(pszParam, fParam);
					m_fEasing = fParam / 100;	// convert percentage to fraction
					break;
				case FLAG_record:
					m_sRecFolderPath = pszParam;
					break;
				case FLAG_recsize:
					Scan(pszParam, m_szRecFrameSize, CSize(1, 1), CSize(INT_MAX, INT_MAX));
					break;
				case FLAG_recrate:
					Scan(pszParam, m_fRecFrameRate, 1e-3f, 1e3f);	// avoid divide by zero
					break;
				case FLAG_recdur:
					Scan(pszParam, m_fRecDuration);
					break;
				case FLAG_markdown:
					m_sHelpMarkdownPath = pszParam;
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
			m_fOutTransDuration = m_fInTransDuration;	// same as incoming
		if (!m_bHasRandSeed)	// if random number seed not specified
			m_nRandSeed = GetTickCount();	// seed from time so runs vary
	}
}

bool CParamParser::ParseCommandLine()
{
	theApp.ParseCommandLine(*this);	// parse command line
	if (m_bError)	// if errors occurred
		return false;	// exit app
	if (m_bShowHelp) {	// if help requested
		if (AttachStdoutToParentConsole()) {	// if console available
			ShowHelp();	// show help on console
		} else {	// no console
			CHelpDlg	dlg;	// show dialog instead
			dlg.DoModal();
		}
		return false;	// exit app
	}
	if (m_bShowLicense) {	// if license requested
		if (AttachStdoutToParentConsole()) {	// if console available
			ShowLicense();	// show license on console
		} else {	// no console
			CAboutDlg	dlg;	// show dialog instead
			dlg.DoModal();
		}
		return false;	// exit app
	}
	if (!m_sHelpMarkdownPath.IsEmpty()) {	// if help markdown requested
		WriteHelpMarkdown(m_sHelpMarkdownPath);	// write help markdown
		return false;	// exit app
	}
	if (!m_strFileName.IsEmpty()) {	// if file path specified
		ReadSlogans(m_strFileName);	// read slogans
	} else {	// no slogan file
		m_aSlogan.Add(m_sDefaultSlogan);	// add placeholder
	}
	return true;	// proceed with launching app
}

void CParamParser::BreakIntoLines(CString sText, CStringArrayEx& arrLine, int nMaxLine)
{
	arrLine.RemoveAll();	// empty caller's array just in case
	CString	sOutLine;
	CString	sWord;
	int	iStart = 0;
	while (!(sWord = sText.Tokenize(_T(" "), iStart)).IsEmpty()) {	// while words remain
		int	nWordLen = sWord.GetLength();
		if (!sOutLine.IsEmpty()) {	// if current line isn't empty
			nWordLen++;
		}
		if (sOutLine.GetLength() + nWordLen >= nMaxLine) {	// if word doesn't fit
			arrLine.Add(sOutLine);	// add finished line to caller's array
			sOutLine.Empty();	// empty current line 
		}
		if (!sOutLine.IsEmpty()) {	// if current line isn't empty
			sOutLine += ' ';	// add separator to current line
		}
		sOutLine += sWord;	// add word to current line
	}
	if (!sOutLine.IsEmpty()) {	// if current line isn't empty
		arrLine.Add(sOutLine);	// add final line to caller's array
	}
}

CString CParamParser::UnpackHelp(CString& sParam, int nParamHelpResID, bool bArgumentUpperCase)
{
	CString	sHelp(LDS(nParamHelpResID));
	bool	bHaveParamName = !sParam.IsEmpty();
	if (bHaveParamName) {	// if parameter name was specified
		sParam.Insert(0, '-');	// insert parameter prefix
	}
	// parameter's help may specify an argument enclosed in square brackets
	if (sHelp[0] == '[') {	// if parameter help specifies an argument
		int	iDelimiter = sHelp.Find(']', 1);	// find closing bracket
		if (iDelimiter >= 0) {	// if argument found
			CString	sArg;
			sArg = sHelp.Mid(1, iDelimiter - 1);	// extract argument
			if (bArgumentUpperCase)
				sArg.MakeUpper();	// make argument upper case
			if (bHaveParamName) {	// if parameter name was specified
				sParam += ' ';	// add separator
			}
			sParam += sArg;	// concatenate argument to parameter name
			sHelp = sHelp.Mid(iDelimiter + 2);	// remove argument from help string
		}
	}
	return sHelp;
}

CString	CParamParser::UnpackHelp(LPCTSTR pszParam, int nParamHelpResID, bool bArgumentUpperCase)
{
	CString	sParam(pszParam);
	CString	sHelp = UnpackHelp(sParam, nParamHelpResID, bArgumentUpperCase);
	if (!sParam.IsEmpty())
		sHelp.Insert(0, sParam + '\t');
	return sHelp;
}

void CParamParser::ShowParamHelp(LPCTSTR pszParamName, int nParamHelpResID, bool bArgumentUpperCase)
{
	CString	sParam(pszParamName);
	CString	sHelp(UnpackHelp(sParam, nParamHelpResID, bArgumentUpperCase));
	CStringArrayEx	arrLine;
	const int	nMaxLine = 80;	// maximum line length in chars
	const int	nMaxParam = 19;	// maximum parameter length in chars
	BreakIntoLines(sHelp, arrLine, nMaxLine - nMaxParam - 1);
	for (int iLine = 0; iLine < arrLine.GetSize(); iLine++) {	// for each line of help text
		// field length is negative so that parameter is left-adjusted
		_tprintf(_T("%*s %s\n"), -nMaxParam, sParam.GetString(), arrLine[iLine].GetString());
		sParam.Empty();	// so continuation lines don't repeat parameter
	}
}

CString	CParamParser::GetAppVersionString()
{
	VS_FIXEDFILEINFO	infoApp;
	CVersionInfo::GetFileInfo(infoApp, NULL);
	CString	sVersion;
	sVersion.Format(_T("%d.%d.%d.%d"), 
		HIWORD(infoApp.dwFileVersionMS), LOWORD(infoApp.dwFileVersionMS),
		HIWORD(infoApp.dwFileVersionLS), LOWORD(infoApp.dwFileVersionLS));
	return sVersion;
}

CString CParamParser::GetAppNameVersionString()
{
	return CString(theApp.m_pszAppName) + ' ' + GetAppVersionString();
}

void CParamParser::ShowAppNameVersion()
{
	_putts(GetAppNameVersionString().GetString());
}

void CParamParser::ShowLicense()
{	
	ShowAppNameVersion();
	CString	sCopyright(m_pszCopyrightNotice);
	sCopyright += '\n';
	_putts(sCopyright.GetString());
	CString	sLicense(LDS(IDS_APP_LICENSE));
	CStringArrayEx	arrLine;
	BreakIntoLines(sLicense, arrLine);
	for (int iLine = 0; iLine < arrLine.GetSize(); iLine++) {
		_putts(arrLine[iLine]);
	}
}

void CParamParser::ShowHelp()
{
	ShowAppNameVersion();
	CString	sHelpUsage(LDS(IDS_HELP_USAGE) + '\n');
	_putts(sHelpUsage.GetString());
	ShowParamHelp(_T(""), IDS_HELP_PARAM_path);	// show path help
	for (int iFlag = 0; iFlag < FLAGS; iFlag++) {	// for each flag
		ShowParamHelp(m_aFlag[iFlag], m_aFlagHelpID[iFlag]);
	}
	CString	sExampleHead('\n' + LDS(IDS_HELP_EXAMPLES));
	_putts(sExampleHead.GetString());	// show some examples too
	for (int iExample = 0; iExample < EXAMPLES; iExample++) {	// for each example
		ShowParamHelp(_T(""), m_aFlagExampleID[iExample], false);
	}
}

CString	CParamParser::GetHelpString()
{
	CString	sHelp(GetAppNameVersionString() + '\n');
	sHelp += LDS(IDS_HELP_USAGE) + '\n';
	sHelp += UnpackHelp(_T(""), IDS_HELP_PARAM_path) + '\n';	// add path help
	for (int iFlag = 0; iFlag < FLAGS; iFlag++) {	// for each flag
		sHelp += UnpackHelp(m_aFlag[iFlag], m_aFlagHelpID[iFlag]) + '\n';
	}
	sHelp += CString('\n' + LDS(IDS_HELP_EXAMPLES)) + '\n';
	for (int iExample = 0; iExample < EXAMPLES; iExample++) {	// for each example
		sHelp += UnpackHelp(_T(""), m_aFlagExampleID[iExample], false) + '\n';
	}
	return sHelp;
}

void CParamParser::WriteHelpMarkdown(LPCTSTR pszOutputPath)
{
	CStdioFile	fOut(pszOutputPath, CFile::modeCreate | CFile::modeWrite);
	fOut.WriteString(_T("### ") + LDS(IDS_HELP_USAGE) + _T("\n\n"));
	fOut.WriteString(_T("|Option|Description|\n|---|---|\n"));
	WriteParamHelpMarkdown(fOut, _T(""), IDS_HELP_PARAM_path);	// show path help
	for (int iFlag = 0; iFlag < FLAGS; iFlag++) {	// for each flag
		WriteParamHelpMarkdown(fOut, m_aFlag[iFlag], m_aFlagHelpID[iFlag]);
	}
	fOut.WriteString(_T("\n### Examples\n\n"));
	fOut.WriteString(_T("|Example|Description|\n|---|---|\n"));
	for (int iExample = 0; iExample < EXAMPLES; iExample++) {	// for each example
		WriteParamHelpMarkdown(fOut, _T(""), m_aFlagExampleID[iExample], false);
	}
}

void CParamParser::WriteParamHelpMarkdown(CStdioFile& fOut, LPCTSTR pszParamName, int nParamHelpResID, bool bArgumentUpperCase)
{
	CString	sParam(pszParamName);
	CString	sHelp(UnpackHelp(sParam, nParamHelpResID, bArgumentUpperCase));
	sParam.Replace(_T(" "), _T("&nbsp;"));	// non-breaking space
	sParam.Replace(_T("-"), _T("&#8209;"));	// non-breaking hyphen
	fOut.WriteString('|' + sParam + '|' + sHelp + _T("|\n"));
}
