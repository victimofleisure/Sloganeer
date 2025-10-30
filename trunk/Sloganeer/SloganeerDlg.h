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

// SloganeerDlg.h : header file
//

#pragma once

#include "SloganDraw.h"

// CSloganeerDlg dialog

class CSloganeerDlg : public CDialogEx, public CRenderThreadBase
{
// Construction
public:
	CSloganeerDlg(CSloganParams& params, CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_SLOGANEER_DIALOG };

// Operations
	bool	ParamCmdLine();

// Implementation
protected:
// Types

// Constants
	enum {
		IDM_ABOUTBOX = 0x0010,
		IDM_FULLSCREEN = 0x0020,
	};

// Member data
	HICON m_hIcon;
	CSloganDraw	m_sd;	// slogan drawing class

// Overrides
	virtual BOOL OnInitDialog();
	virtual BOOL DestroyWindow();

// Message handlers
	DECLARE_MESSAGE_MAP()
	afx_msg void OnPaint();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
};
