// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      24oct25	initial version

*/

// Sloganeer.h : main header file for the application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CSloganeerApp:
// See Sloganeer.cpp for the implementation of this class
//

class CSloganeerApp : public CWinApp
{
public:
	CSloganeerApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Attributes
	static	CString GetVersionString();

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

extern CSloganeerApp theApp;
