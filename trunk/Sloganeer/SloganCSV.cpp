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

#include "stdafx.h"
#include "Sloganeer.h"
#include "SloganParams.h"
#include "SloganCSV.h"
#include "StdioFileEx.h"
#include "ParseCSV.h"
#include "ParamParser.h"
#include <exception>

// Most slogan columns have the same name as a command line flag, and that
// lets our base class (CParamParser) parse them, avoiding duplicate code.
// If this array gives compiler errors (undeclared indentifier), check the
// SLOGANDEF macros in ParamDef.h and verify that all names also appear in
// PARAMDEF, or are intentionally different because they're not command line
// flags. Non-flag columns must define an invalid flag in our header file.
//
const int CSloganCSV::m_iColumnFlag[COLUMNS] = {
	#define SLOGANDEF(name, member) FLAG_##name,
	#include "ParamDef.h"	// generate code
};

CSloganCSV::CSloganCSV(const CSlogan& sloganDefaults, CSloganArray& aSlogan) 
	: m_aSlogan(aSlogan)	// store reference to destination slogan array
{
	CSlogan::operator=(sloganDefaults);	// copy slogan defaults to base class
	m_iLine = 0;
	m_iSelCol = 0;
}

void CSloganCSV::OnError(int nErrID, LPCTSTR pszParam, LPCTSTR pszErrInfo)
{
	ASSERT(nErrID > 0);
	ASSERT(pszParam != NULL);
	CString	sErrMsg;
	AfxFormatString1(sErrMsg, nErrID, pszParam);	// format error message
	if (pszErrInfo != NULL)
		sErrMsg += '\n' + CString(pszErrInfo);
	CString	sErrLoc;
	// error location is one-based line and one-based column
	sErrLoc.Format(IDS_CSV_ERR_LOCATION, m_iLine + 1, m_iSelCol + 1);
	AfxMessageBox(sErrMsg + '\n' + sErrLoc);	// report error and its location
	m_bError = true;	// set error state
}

void CSloganCSV::SetDefaultColumnLayout()
{
	m_aSelCol.SetSize(COLUMNS);	// all columns are selected, in order
	for (int iCol = 0; iCol < COLUMNS; iCol++) {	// for each column
		m_aSelCol[iCol] = iCol;	// set selected column to column index
	}
}

bool CSloganCSV::CheckForHeaderRow(CString sLine, bool& bHasHeaderRow)
{
	bHasHeaderRow = false;	// assume no header row
	ASSERT(!m_iLine);	// first row only, else logic error
	// set default column layout in case there's no header row
	SetDefaultColumnLayout();
	bool	bColNamesFound = false;
	CString	sUnkCol;	// name of first unknown column
	int	iUnkCol = -1;	// header index of first unknown column
	int	iHdrCol = 0;	// current index of column within header
	CIntArrayEx	aSelCol;	// local column selection
	CParseCSV	csv(sLine);	// create parser for CSV line
	CString	sToken;
	while (csv.GetString(sToken)) {	// for each token
		sToken.Trim();	// remove leading and trailing whitespace
		// if token is empty or starts with a digit
		if (sToken.IsEmpty() || isdigit(sToken[0]))
			return true;	// no header row, success
		int	iCol = FindColumnName(sToken);	// look up column name
		if (iCol >= 0) {	// if column name found
			// text column is too ambiguous to be proof of header row
			if (iCol != COL_text)	// if any column except text
				bColNamesFound = true;	// set found flag
		} else {	// unknown column name
			if (iUnkCol < 0) {	// if first unknown column
				sUnkCol = sToken;	// save unknown column name
				iUnkCol = iHdrCol;	// save header column index
			}
		}
		aSelCol.Add(iCol);	// add column index to selection
		iHdrCol++;	// bump header column index
	}
	if (!bColNamesFound)	// if no column names were found
		return true;	// no header row, success
	// header row detected; validate column selection
	int	nSelCols = aSelCol.GetSize();
	for (int iSelCol = 0; iSelCol < nSelCols; iSelCol++) {	// for each selected column
		if (aSelCol[iSelCol] < 0) {	// if selected column is invalid
			m_iSelCol = iUnkCol;	// set column index for error reporting
			// OnError sets error state to indicate a serious error
			OnError(IDS_ERR_BAD_COLUMN_NAME, sUnkCol);	// report error
			return false;	// invalid header row, fail
		}
	}
	bHasHeaderRow = true;	// header row successfully parsed
	m_aSelCol.Swap(aSelCol);	// move column selection to member
	return true;	// success
}

bool CSloganCSV::ParseLine(CString sLine)
{
	CParseCSV	csv(sLine);	// create parser for CSV line
	int	iTransDir;
	int	nSelCols = m_aSelCol.GetSize();
	for (int iSelCol = 0; iSelCol < nSelCols; iSelCol++) {	// for each selected column
		m_iSelCol = iSelCol;	// stash selected column index for error reporting
		int	iCol = m_aSelCol[iSelCol];
		try {
			CString	sToken;
			if (!csv.GetString(sToken))	// if no more tokens
				break;	// we're done
			if (sToken.IsEmpty())	// if token is empty
				continue;	// skip this member
			switch (iCol) {	// switch on column index
			case COL_text:
				if (!EscapeChars(sToken)) {	// handle escape sequences
					OnError(IDS_ERR_BAD_ESCAPE_SEQ, sToken);
					return false;
				}
				m_sText = sToken;	// set slogan text
				break;
			case COL_intrans:
			case COL_outtrans:
				// deduce transition direction from column index
				iTransDir = (iCol == COL_outtrans);
				// parse token to an incoming or outgoing transition type
				if (!ParseTransType(sToken, m_aTransType[iTransDir])) {
					OnError(IDS_ERR_PARAM_RANGE, m_aColumnName[iCol]);	// report error
					return false;	// abort parsing
				}
				break;
			default:
				sToken.Trim();	// remove leading and trailing whitespace
				// assume remaining columns are also command line flags, and
				// reuse parameter parser for them, avoiding code duplication
				m_bError = false;	// reset error flag
				ParseParam(m_aColumnName[iCol], true, false);	// parse column name
				ParseParam(sToken, false, false);	// parse column value
				if (m_bError)	// if parsing failed
					return false;	// abort parsing
			}
			m_nCustomColMask |= 1 << iCol;	// mark column as customized
		} catch (const std::exception& e) {	// std conversion functions throw exception
			OnError(IDS_ERR_PARAM_INVALID, m_aColumnName[iCol], CString(e.what()));	// report error
			return false;	// abort parsing
		}
	}
	if (!m_bHasOutDur)	// if outgoing transition duration not specified
		m_fOutTransDuration = m_fInTransDuration;	// same as incoming
	return true;	// line successfully parsed
}

bool CSloganCSV::Read(LPCTSTR pszPath)
{
	m_iLine = 0;
	CStdioFileEx	fIn(pszPath, CFile::modeRead);	// open CSV file
	CString	sLine;
	CSlogan	sloganDefaults(*this);	// copy slogan defaults
	while (fIn.ReadString(sLine)) {	// for each line
		if (!m_iLine) {	// if first line
			// check for header row
			bool	bHeaderRow;
			if (!CheckForHeaderRow(sLine, bHeaderRow)) {
				return false;	// parsing failed, abort
			}
			if (bHeaderRow) {	// if file has header row
				m_iLine++;	// bump line index
				continue;	// skip to next line
			}
		}
		if (sLine.IsEmpty())	// if line is empty
			continue;	// skip completely empty lines
		CSlogan::operator=(sloganDefaults);	// set slogan defaults
		if (!ParseLine(sLine))	// parse line into slogan members
			return false;	// if parsing failed, abort
		m_aSlogan.Add(*this);	// add slogan to array
		m_iLine++;	// bump line index
	}
	return true;	// file successfully read
}
