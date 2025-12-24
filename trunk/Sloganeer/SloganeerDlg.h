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
		02		14nov25	add delayed create handler
		03		11dec25	add parameters dialog
		04		24dec25	add file open command

*/

// SloganeerDlg.h : header file
//

#pragma once

#include "SloganDraw.h"
#include "ParamsDlg.h"

// CSloganeerDlg dialog

class CSloganeerDlg : public CDialogEx
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
		IDM_ABOUTBOX	= 0x0010,
		IDM_FULLSCREEN	= 0x0020,
		IDM_SHOWHELP	= 0x0030,
		IDM_SHOWPARAMS	= 0x0040,
		IDM_OPENFILE	= 0x0050,
	};

// Member data
	HICON m_hIcon;
	CSlogan	m_initSlogan;	// initial slogan state
	CSloganDraw	m_sd;	// slogan drawing class
	CParamsDlg	m_dlgParams;	// parameters dialog
	bool	m_bShowParamsDlg;	// true if showing parameters dialog

// Overrides
	virtual BOOL OnInitDialog();
	virtual BOOL DestroyWindow();
	virtual void OnOK();
	virtual void OnCancel();

// Helpers
	bool	CustomizeSystemMenu();
	bool	Record();
	void	ShowHelp();
	void	FullScreen(bool bEnable);
	bool	HandleKey(UINT nKey);
	bool	ShowParamsDlg(bool bEnable);
	bool	IsParamsDlgVisible() const;
	bool	PromptOpenInputFile();
	friend class CParamsDlg;

// Message handlers
	DECLARE_MESSAGE_MAP()
	afx_msg void OnPaint();
	afx_msg void OnClose();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg LRESULT OnDelayedCreate(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnFullScreenChanged(WPARAM wParam, LPARAM lParam);
};
