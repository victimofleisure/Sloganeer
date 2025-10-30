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

// Sloganeer.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "Sloganeer.h"
#include "SloganeerDlg.h"
#include "Win32Console.h"
#include "VersionInfo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CSloganeerApp

BEGIN_MESSAGE_MAP(CSloganeerApp, CWinApp)
END_MESSAGE_MAP()

// CSloganeerApp construction

CSloganeerApp::CSloganeerApp()
{
}

// The one and only CSloganeerApp object

CSloganeerApp theApp;

// CSloganeerApp initialization

BOOL CSloganeerApp::InitInstance()
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

#ifdef _DEBUG
	Win32Console::Create();
#endif

	AfxEnableControlContainer();

	// Activate "Windows Native" visual manager for enabling themes in MFC controls
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

	SetRegistryKey(_T("Anal Software"));

	CSloganParams	params;
	CParamsParser	parser(params);
	if (!parser.Parse())
		return false;

	CSloganeerDlg dlg(params);
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == -1)
	{
		TRACE(traceAppMsg, 0, "Warning: dialog creation failed, so application is terminating unexpectedly.\n");
		TRACE(traceAppMsg, 0, "Warning: if you are using MFC controls on the dialog, you cannot #define _AFX_NO_MFC_CONTROLS_IN_DIALOGS.\n");
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

CString CSloganeerApp::GetVersionString()
{
	VS_FIXEDFILEINFO	AppInfo;
	CString	sVersion;
	CVersionInfo::GetFileInfo(AppInfo, NULL);
	sVersion.Format(_T("%d.%d.%d.%d"), 
		HIWORD(AppInfo.dwFileVersionMS), LOWORD(AppInfo.dwFileVersionMS),
		HIWORD(AppInfo.dwFileVersionLS), LOWORD(AppInfo.dwFileVersionLS));
#ifdef _WIN64
	sVersion += _T(" x64");
#else
	sVersion += _T(" x86");
#endif
#ifdef _DEBUG
	sVersion += _T(" Debug");
#else
	sVersion += _T(" Release");
#endif
	return sVersion;
}
