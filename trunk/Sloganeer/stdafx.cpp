// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      26oct25	initial version

*/

// stdafx.cpp : source file that includes just the standard includes
// Sloganeer.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"
#include "resource.h"

CString FormatSystemError(DWORD ErrorCode)
{
	LPTSTR	lpszTemp;
	DWORD	bRet = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, ErrorCode, 0, (LPTSTR)&lpszTemp, 0, NULL);	// default language
	CString	sError;	
	if (bRet) {	// if format succeeded
		sError = lpszTemp;	// create string from message buffer
		LocalFree(lpszTemp);	// free message buffer
	} else	// format failed
		sError.Format(IDS_APP_UNKNOWN_SYSTEM_ERROR, ErrorCode, ErrorCode);
	return(sError);
}

CString	GetLastErrorString()
{
	return(FormatSystemError(GetLastError()));
}

bool FilesEqual(LPCTSTR pszFile1, LPCTSTR pszFile2)
{
	CFile f1(pszFile1, CFile::modeRead);
	CFile f2(pszFile2, CFile::modeRead);
	UINT nSize = (UINT)f1.GetLength();
	if (nSize != (UINT)f2.GetLength())
		return false;
	CByteArray b1, b2;
	b1.SetSize(nSize);
	b2.SetSize(nSize);
	f1.Read(b1.GetData(), nSize);
	f2.Read(b2.GetData(), nSize);
	return !memcmp(b1.GetData(), b2.GetData(), nSize);
}
