// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      11dec25	initial version

*/

#pragma once

// CParamsDlg dialog

class CSloganeerDlg;

class CParamsDlg : public CDialog, public CSlogan
{
	DECLARE_DYNAMIC(CParamsDlg)

public:
	CParamsDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CParamsDlg();

// Dialog Data
	enum { IDD = IDD_PARAMS };

protected:
// Data members
	CSloganeerDlg*	m_pParentDlg;
	CComboBox	m_cbSlogan;
	CComboBox	m_cbFontName;
	CMFCColorButton	m_btnBkgnd;
	CMFCColorButton	m_btnDraw;
	CComboBox	m_cbTransType[TRANS_DIRS];
	int		m_iSloganPlayMode;
	int		m_iSelSlogan;
	BOOL	m_bImmediateMode;

// Helpers
	bool	InitFontFamilyCombo();
	static void	GetTransTypeNames(CStringArrayEx& aTypeName);
	static void	InitTransTypeCombo(CComboBox& cb, int iSelTransType, const CStringArrayEx& aTypeName);
	void	UpdateTransType(int iDir);
	void	PushSlogan(CSlogan& src, UINT nColMask);

// Overrides
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

// Message map
	DECLARE_MESSAGE_MAP()
	afx_msg void OnClose();
	afx_msg void OnSelendokSlogan();
	afx_msg void OnKillfocusSlogan();
	afx_msg void OnSelendokFontName();
	afx_msg void OnKillfocusFontSize();
	afx_msg void OnKillfocusFontWeight();
	afx_msg void OnClickedColorBkgnd();
	afx_msg void OnClickedColorDraw();
	afx_msg void OnKillfocusColorBkgndAlpha();
	afx_msg void OnKillfocusColorDrawAlpha();
	afx_msg void OnClickedPlayMode(UINT nID);
	afx_msg void OnKillfocusDurIn();
	afx_msg void OnKillfocusDurHold();
	afx_msg void OnKillfocusDurOut();
	afx_msg void OnKillfocusDurPause();
	afx_msg void OnSelendokTransTypeIn();
	afx_msg void OnSelendokTransTypeOut();
	afx_msg void OnClickedImmediateMode();
public:
	afx_msg void OnClickedSloganNew();
};
