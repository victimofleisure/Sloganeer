// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      24oct25	initial version
        01      01nov25	add error log

*/

// Sloganeer.h : main header file for the application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols
#include "CritSec.h"
#include "WinAppCK.h"

// CSloganeerApp:
// See Sloganeer.cpp for the implementation of this class
//

class CSloganeerApp : public CWinAppCK
{
public:
	CSloganeerApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Operations
	bool	WriteLogEntry(CString sLogEntry);

// Implementation
protected:
	WCritSec	m_csErrorLog;		// critical section for error log file

	DECLARE_MESSAGE_MAP()
};

extern CSloganeerApp theApp;
