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

// SloganeerDlg.h : header file
//

#pragma once

#include "SloganDraw.h"

// CSloganeerDlg dialog

class CSloganeerDlg : public CDialogEx, public CRenderThreadBase
{
// Construction
public:
	CSloganeerDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_SLOGANEER_DIALOG };

// Operations
	bool	ParamCmdLine();

// Implementation
protected:
// Types
	class CMyCommandLineInfo : public CCommandLineInfo {
	public:
		CMyCommandLineInfo(CSloganeerDlg &dlg);
		enum {
			#define PARAMDEF(name) FLAG_##name,
			#include "ParamDef.h"
			FLAGS
		};
		static const LPCTSTR m_aFlag[FLAGS];
		virtual void ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast);
		CSloganeerDlg&	m_dlg;	// parent dialog
		int		m_iFlag;	// index of flag expecting a parameter
		bool	m_bError;	// true if error occurred
		void	OnError(int nErrID, LPCTSTR pszParam);
	};

// Constants
	enum {
		IDM_ABOUTBOX = 0x0010,
		IDM_FULLSCREEN = 0x0020,
	};
	static const LPCTSTR m_aSlogan[];	// default array of slogans

// Member data
	HICON m_hIcon;
	CSloganDraw	m_sd;	// slogan drawing class
	bool	m_bStartFullScreen;	// true if starting in full screen mode

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
