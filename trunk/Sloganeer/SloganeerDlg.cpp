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
		02		14nov25	add recording
		03		03dec25	add recording to named pipe
		04		11dec25	add parameters dialog
		05		24dec25	add file open command

*/

// SloganeerDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Sloganeer.h"
#include "SloganeerDlg.h"
#include "AboutDlg.h"
#include "ProgressDlg.h"
#include "PathStr.h"
#include "HelpDlg.h"
#include "ParamParser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CSloganeerDlg dialog

CSloganeerDlg::CSloganeerDlg(CSloganParams& params, CWnd* pParent /*=NULL*/)
	: m_sd(params), CDialogEx(CSloganeerDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_bShowParamsDlg = false;
}

class CDlgSloganDraw : public CSloganDraw {
public:
// Construction
	CDlgSloganDraw::CDlgSloganDraw(const CSloganParams& params);
	bool	Create();

// Overrides
	virtual bool	OnDraw();

// Member data
protected:
	CProgressDlg	m_dlgProgress;
	UINT	m_nRecDurFrames;
};

CDlgSloganDraw::CDlgSloganDraw(const CSloganParams& params) : CSloganDraw(params)
{
}

bool CDlgSloganDraw::Create()
{
	if (!m_dlgProgress.Create())	// if can't create progress bar
		return false;
	m_nRecDurFrames = Round(m_fRecDuration * m_fRecFrameRate);
	m_dlgProgress.SetRange(0, m_nRecDurFrames);
	return Record();
}

bool CDlgSloganDraw::OnDraw()
{
	m_dlgProgress.SetPos(m_iFrame);
	if (m_dlgProgress.Canceled())	// if user canceled
		return false;	// abort recording
	return CSloganDraw::OnDraw();
}

bool CSloganeerDlg::Record()
{
	CString	sRecFolderPath(m_sd.GetParams().m_sRecFolderPath);
	if (!CSloganParams::IsPipeName(sRecFolderPath)) {	// if path isn't a named pipe
		// check for pre-existing image sequence in export folder
		CString	sImagePath(CSloganDraw::MakeImageSequenceFileName(sRecFolderPath, 0));
		if (PathFileExists(sImagePath)) {	// if first frame exists
			UINT	nType = MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING;
			if (AfxMessageBox(IDS_IMAGE_SEQ_OVERWRITE_WARN, nType) != IDYES)
				return false;	// user chickened out
		}
		// delete pre-existing image sequence if any
		CString	sRecordExt(PathFindExtension(sImagePath));
		CPathStr	sWildcardPath(sRecFolderPath);
		sWildcardPath.Append('*' + sRecordExt);
		WildcardDeleteFile(sWildcardPath);
	}
	// construct local slogan draw instance
	CDlgSloganDraw	sd(m_sd.GetParams());
	sd.Create();
	return true;
}

bool CSloganeerDlg::CustomizeSystemMenu()
{
	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);
	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu == NULL)
		return false;
	CString sMenuItem;
	VERIFY(sMenuItem.LoadString(IDS_FULL_SCREEN));
	VERIFY(pSysMenu->InsertMenu(SC_MINIMIZE, 0, IDM_FULLSCREEN, sMenuItem));
	VERIFY(pSysMenu->AppendMenu(MF_SEPARATOR));
	VERIFY(sMenuItem.LoadString(IDS_OPEN_FILE));
	VERIFY(pSysMenu->AppendMenu(MF_STRING, IDM_OPENFILE, sMenuItem));
	VERIFY(sMenuItem.LoadString(IDS_PARAMS));
	VERIFY(pSysMenu->AppendMenu(MF_STRING, IDM_SHOWPARAMS, sMenuItem));
	VERIFY(pSysMenu->AppendMenu(MF_SEPARATOR));
	VERIFY(sMenuItem.LoadString(IDS_HELP));
	VERIFY(pSysMenu->AppendMenu(MF_STRING, IDM_SHOWHELP, sMenuItem));
	VERIFY(sMenuItem.LoadString(IDS_ABOUTBOX));
	VERIFY(pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, sMenuItem));
	return true;
}

void CSloganeerDlg::ShowHelp()
{
	CHelpDlg	dlgHelp;
	dlgHelp.DoModal();
}

void CSloganeerDlg::FullScreen(bool bEnable)
{
	if (bEnable == m_sd.IsFullScreen())	// if already in requested state
		return;	// nothing to do
	if (bEnable) {	// if entering full screen
		// can't go full screen exclusive with child dialogs visible,
		// so save their visibility and then hide them
		m_bShowParamsDlg = IsParamsDlgVisible();
		if (m_bShowParamsDlg) {
			m_dlgParams.ShowWindow(SW_HIDE);
			SetFocus();
		}
	}
	m_sd.FullScreen(bEnable);
}

bool CSloganeerDlg::ShowParamsDlg(bool bEnable)
{
	if (!m_dlgParams.m_hWnd) {	// if parameters dialog not created
		if (!m_dlgParams.Create(IDD_PARAMS))	// create dialog
			return false;
	}
	m_dlgParams.ShowWindow(bEnable ? SW_SHOW : SW_HIDE);
	return true;
}

bool CSloganeerDlg::IsParamsDlgVisible() const
{
	// dialog may not exist yet, so check window handle first
	return m_dlgParams.m_hWnd && m_dlgParams.IsWindowVisible();
}

bool CSloganeerDlg::PromptOpenInputFile()
{
	static const LPCTSTR pszFilter = _T("Input Files (*.txt;*.csv)|*.txt;*.csv|All Files (*.*)|*.*||");
	CFileDialog	fd(TRUE, NULL, NULL, OFN_HIDEREADONLY, pszFilter);
	if (fd.DoModal() != IDOK)
		return false;
	CParamParser	parser;
	if (!parser.OpenInputFile(fd.GetPathName()))
		return false;
	m_sd.SetSloganArray(parser.m_aSlogan, parser.m_bCustomSlogans);
	m_dlgParams.SetSlogans(parser.m_aSlogan);
	return true;
}

bool CSloganeerDlg::HandleKey(UINT nKey)
{
	switch (nKey) {
	case VK_F11:
		FullScreen(!m_sd.IsFullScreen());
		break;
	case VK_F1:
		ShowHelp();
		break;
	case VK_F5:
		ShowParamsDlg(!IsParamsDlgVisible());
		break;
	case 'O':
		if ((GetKeyState(VK_CONTROL) & GKS_DOWN)
		&& !(GetKeyState(VK_SHIFT) & GKS_DOWN)
		&& !(GetKeyState(VK_MENU) & GKS_DOWN))
			PromptOpenInputFile();
		break;
	default:
		return false;
	}
	return true;
}

BEGIN_MESSAGE_MAP(CSloganeerDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_CLOSE()
	ON_WM_KEYDOWN()
	ON_WM_SIZE()
	ON_WM_SETCURSOR()
	ON_WM_SYSCOMMAND()
	ON_MESSAGE(UWM_DELAYED_CREATE, OnDelayedCreate)
	ON_MESSAGE(UWM_FULL_SCREEN_CHANGED, OnFullScreenChanged)
END_MESSAGE_MAP()

// CSloganeerDlg message handlers

BOOL CSloganeerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CustomizeSystemMenu();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	m_initSlogan = m_sd.GetParams();	// copy initial slogan state
	PostMessage(UWM_DELAYED_CREATE);
	if (m_sd.GetParams().IsRecording()) {	// if recording
		return TRUE;	// no further creation
	}
	if (!m_sd.Create(m_hWnd, this))	// create slogan drawing object
		return TRUE;
	m_sd.FullScreen(m_sd.GetParams().m_bStartFullScreen);	// go full screen if requested

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CSloganeerDlg::OnOK()
{
}

void CSloganeerDlg::OnCancel()
{
}

void CSloganeerDlg::OnClose()
{
	EndDialog(IDOK);
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
		FullScreen(!m_sd.IsFullScreen());
		break;
	case IDM_SHOWHELP:
		ShowHelp();
		break;
	case IDM_SHOWPARAMS:
		ShowParamsDlg(!IsParamsDlgVisible());
		break;
	case IDM_OPENFILE:
		PromptOpenInputFile();
		break;
	default:
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

void CSloganeerDlg::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (!HandleKey(nChar))
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

LRESULT CSloganeerDlg::OnDelayedCreate(WPARAM wParam, LPARAM lParam)
{
	if (m_sd.GetParams().IsRecording()) {	// if recording
		Record();
		PostMessage(WM_CLOSE);	// exit app regardless; OnOK is disabled
		return 0;
	}
	return 0;
}

LRESULT CSloganeerDlg::OnFullScreenChanged(WPARAM wParam, LPARAM lParam)
{
	if (!wParam) {	// if windowed
		// restore child windows
		if (m_bShowParamsDlg)
			m_dlgParams.ShowWindow(SW_SHOW);
	}
	return 0;
}
