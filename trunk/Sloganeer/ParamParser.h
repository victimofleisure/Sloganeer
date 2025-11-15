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

#pragma once

#include "SloganParams.h"

class CParamParser : public CSloganParams, public CCommandLineInfo {
public:
// Construction
	CParamParser();
	virtual ~CParamParser();

// Operations
	bool	ParseCommandLine();
	static	CString	GetHelpString();

protected:
// Constants
	enum {
		#define PARAMDEF(name) FLAG_##name,
		#include "ParamDef.h"
		FLAGS
	};
	enum {
		#define HELPEXAMPLEDEF(name) EXAMPLE_##name,
		#include "ParamDef.h"
		EXAMPLES
	};
	static const LPCTSTR m_aFlag[FLAGS];	// array of flag name strings
	static const int m_aFlagHelpID[FLAGS];	// array of flag help string resource IDs
	static const int m_aFlagExampleID[EXAMPLES];	// array of flag example string resource IDs
	static const LPCTSTR m_pszCopyrightNotice;	// copyright notice
	static const LPCTSTR m_sDefaultSlogan;	// default slogan if none are specified

// Overrides
	virtual void ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast);

// Data members
	int		m_iFlag;			// index of flag expecting a parameter
	bool	m_bError;			// true if error occurred
	bool	m_bHasOutDur;		// true if outgoing duration was specified
	bool	m_bHasRandSeed;		// true if random number seed was specified
	bool	m_bShowHelp;		// true if we should show help
	bool	m_bShowLicense;		// true if we should show licence
	CString	m_sHelpMarkdownPath;	// path for help markdown output file

// Helpers
	template<typename T> void Convert(LPCTSTR pszParam, T& val);
	template<typename T> bool Scan(LPCTSTR pszParam, T& val, T minVal = 0, T maxVal = 0);
	bool	Scan(LPCTSTR pszParam, D2D1::ColorF& color);
	void	OnError(int nErrID, LPCTSTR pszParam);
	static	void	BreakIntoLines(CString sText, CStringArrayEx& arrLine, int nMaxLine = 80);
	static	CString	UnpackHelp(CString& sParam, int nParamHelpResID, bool bArgumentUpperCase = true);
	static	CString	UnpackHelp(LPCTSTR pszParam, int nParamHelpResID, bool bArgumentUpperCase = true);
	static	void	ShowParamHelp(LPCTSTR pszParamName, int nParamHelpResID, bool bArgumentUpperCase = true);
	static	CString	GetAppVersionString();
	static	CString	GetAppNameVersionString();
	static	void	ShowAppNameVersion();
	static	void	ShowLicense();
	static	void	ShowHelp();
	static	void	WriteHelpMarkdown(LPCTSTR pszOutputPath);
	static	void	WriteParamHelpMarkdown(CStdioFile& fOut, LPCTSTR pszParamName, int nParamHelpResID, bool bArgumentUpperCase = true);
};
