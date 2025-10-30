// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      24oct25	initial version
		01		30oct25	move parameters to their own class

*/

// SloganeerDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Sloganeer.h"
#include "SloganeerDlg.h"
#include "AboutDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CSloganeerDlg dialog

CSloganeerDlg::CSloganeerDlg(CSloganParams& params, CWnd* pParent /*=NULL*/)
	: m_sd(params), CDialogEx(CSloganeerDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
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
			// append about box to system menu
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
		CString strFullScreenMenu;
		bNameValid = strFullScreenMenu.LoadString(IDS_FULL_SCREEN);
		ASSERT(strFullScreenMenu);
		if (!strFullScreenMenu.IsEmpty())
		{
			// insert full screen command into system menu, before minimize
			pSysMenu->InsertMenu(SC_MINIMIZE, 0, IDM_FULLSCREEN, strFullScreenMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	m_sd.Create(m_hWnd);	// create slogan drawing object
	m_sd.FullScreen(m_sd.GetParms().m_bStartFullScreen);	// go full screen if requested

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
