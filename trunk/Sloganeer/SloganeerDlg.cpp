// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      24oct25	initial version
		01		29oct25	add pause and color parameters

*/

// SloganeerDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Sloganeer.h"
#include "SloganeerDlg.h"
#include "StdioFileUTF.h"
#include <string>	// for stoi and stof
#include "AboutDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

const LPCTSTR CSloganeerDlg::m_aSlogan[] = {
	_T("YOUR TEXT")
};

const LPCTSTR CSloganeerDlg::CMyCommandLineInfo::m_aFlag[FLAGS] = {
	#define PARAMDEF(name) _T(#name),
	#include "ParamDef.h"
};

CSloganeerDlg::CMyCommandLineInfo::CMyCommandLineInfo(CSloganeerDlg& dlg) : m_dlg(dlg)
{
	m_iFlag = -1;
	m_bError = false;
}

void CSloganeerDlg::CMyCommandLineInfo::OnError(int nErrID, LPCTSTR pszParam)
{
	CString	sErrMsg;
	AfxFormatString1(sErrMsg, nErrID, pszParam);
	AfxMessageBox(sErrMsg);
	m_bError = true;
}

void CSloganeerDlg::CMyCommandLineInfo::ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast)
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
				m_dlg.m_bStartFullScreen = true;
				break;
			case FLAG_seqtext:
				m_dlg.m_sd.SetSloganOrder(true);	// sequential order
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
				m_dlg.m_sd.SetFontSize(std::stof(pszParam));
				break;
			case FLAG_fontname:
				m_dlg.m_sd.SetFontName(pszParam);
				break;
			case FLAG_fontweight:
				m_dlg.m_sd.SetFontWeight(std::stoi(pszParam));
				break;
			case FLAG_transdur:
				m_dlg.m_sd.SetTransDuration(std::stof(pszParam));
				break;
			case FLAG_holddur:
				m_dlg.m_sd.SetHoldDuration(std::stof(pszParam));
				break;
			case FLAG_pausedur:
				m_dlg.m_sd.SetPauseDuration(std::stof(pszParam));
				break;
			case FLAG_bgcolor:
				m_dlg.m_sd.SetBkgndColor(std::stoi(pszParam, 0, 16));	// hexadecimal
				break;
			case FLAG_drawcolor:
				m_dlg.m_sd.SetDrawColor(std::stoi(pszParam, 0, 16));	// hexadecimal
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
	}
}

// CSloganeerDlg dialog

CSloganeerDlg::CSloganeerDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSloganeerDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_bStartFullScreen = false;
}

bool CSloganeerDlg::ParamCmdLine()
{
	CMyCommandLineInfo	cli(*this);
	theApp.ParseCommandLine(cli);
	if (!cli.m_strFileName.IsEmpty()) {	// if file name specified
		// assume UTF-8 text file containing slogans to display, one per line
		CStdioFileUTF	fSlogan;
		if (!fSlogan.Open(cli.m_strFileName, CFile::modeRead)) {	// if open error
			cli.OnError(IDS_ERR_FILE_NOT_FOUND, cli.m_strFileName);
			return false;
		}
		CStringArrayEx	aSlogan;
		CString	sSlogan;
		while (fSlogan.ReadString(sSlogan)) {	// read slogan string
			if (!sSlogan.IsEmpty()) {	// if string isn't empty
				sSlogan.Replace('\t', '\n');	// replace tabs with newlines
				aSlogan.Add(sSlogan);	// add slogan to array
			}
		}
		m_sd.SetSlogans(aSlogan);	// pass slogans to drawing object
	}
	return !cli.m_bError;	// return true if no errors
}

BEGIN_MESSAGE_MAP(CSloganeerDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_KEYDOWN()
	ON_WM_SIZE()
	ON_WM_SETCURSOR()
	ON_WM_SYSCOMMAND()
END_MESSAGE_MAP()

// CSloganeerDlg message handlers

BOOL CSloganeerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
		CString strFullScreenMenu;
		bNameValid = strFullScreenMenu.LoadString(IDS_FULL_SCREEN);
		ASSERT(strFullScreenMenu);
		if (!strFullScreenMenu.IsEmpty())
		{
			pSysMenu->InsertMenu(SC_MINIMIZE, 0, IDM_FULLSCREEN, strFullScreenMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	if (!m_sd.HasSlogans())	// if slogans not already specified
		m_sd.SetSlogans(m_aSlogan, _countof(m_aSlogan));	// set default slogans
	m_sd.Create(m_hWnd);	// create slogan drawing object
	m_sd.FullScreen(m_bStartFullScreen);	// go full screen if requested

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CSloganeerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		ValidateRect(NULL);	// suppress paint; Direct2D does drawing
	}
}

BOOL CSloganeerDlg::DestroyWindow()
{
	m_sd.Destroy();
	return CDialogEx::DestroyWindow();
}

void CSloganeerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	switch (nID & 0xFFF0) {
	case IDM_ABOUTBOX:
		{
			CAboutDlg dlgAbout;
			dlgAbout.DoModal();
		}
		break;
	case IDM_FULLSCREEN:
		m_sd.FullScreen(!m_sd.IsFullScreen());
		break;
	default:
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

void CSloganeerDlg::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar == VK_F11) {
		m_sd.FullScreen(!m_sd.IsFullScreen());
	}
	CDialogEx::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CSloganeerDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);
	m_sd.Resize();
}

BOOL CSloganeerDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (m_sd.IsFullScreen()) {	// if app is full screen
		SetCursor(NULL);	// hide the cursor by loading a null cursor
		return true;	// skip base class behavior to avoid flicker
	}
	return CWnd::OnSetCursor(pWnd, nHitTest, message);
}
