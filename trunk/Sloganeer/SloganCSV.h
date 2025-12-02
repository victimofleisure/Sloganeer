// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      18nov25	initial version
		01		25nov25	improve error reporting

*/

#pragma once

#include "ParamParser.h"

class CSloganParams;

class CSloganCSV : public CParamParser {
public:
// Construction
	CSloganCSV(const CSlogan& sloganDefaults, CSloganArray& aSlogan);

// Operations
	bool	Read(LPCTSTR pszPath);

protected:
// Constants
	static const int m_iColumnFlag[COLUMNS];	// map columns to flags
	enum {	// define non-flag columns to prevent compiler errors
		FLAG_text = INVALID,
		FLAG_intrans = INVALID,
		FLAG_outtrans = INVALID,
	};

// Data members
	CSloganArray&	m_aSlogan;	// reference to slogan array
	CIntArrayEx	m_aSelCol;	// array of selected column indices
	int		m_iLine;	// index of line being parsed
	int		m_iSelCol;	// index of selected column being parsed

// Overrides
	virtual void OnError(int nErrID, LPCTSTR pszParam, LPCTSTR pszErrInfo = NULL);

// Helpers
	void	SetDefaultColumnLayout();
	bool	CheckForHeaderRow(CString sLine, bool& bHasHeaderRow);
	bool	ParseLine(CString sLine);
};
