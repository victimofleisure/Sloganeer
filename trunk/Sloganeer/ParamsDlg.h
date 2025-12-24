// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      11dec25	initial version
		01		14dec25	add manual trigger
		02		24dec25	add set slogans attribute

*/

#pragma once

// CParamsDlg dialog

class CSloganeerDlg;

class CParamsDlg : public CDialog, public CSlogan
{
	DECLARE_DYNAMIC(CParamsDlg)

public:
// Construction
	CParamsDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CParamsDlg();

// Attributes
	void	SetSlogans(const CSloganArray& aSlogan);

// Dialog Data
	enum { IDD = IDD_PARAMS };

protected:
// Constants
	enum {	// trigger types
		TRIG_AUTO,
		TRIG_MANUAL,
		TRIGGER_TYPES
	};

// Data members
	CSloganeerDlg*	m_pAppDlg;	// pointer to app dialog
	CComboBox	m_cbSlogan;		// slogan combo box
	CComboBox	m_cbFontName;	// font combo box
	CMFCColorButton	m_btnBkgnd;	// background color button
	CMFCColorButton	m_btnDraw;	// draw color button
	CComboBox	m_cbTransType[TRANS_DIRS];	// transition type combo boxes
	int		m_iSloganPlayMode;	// slogan play mode radio button index
	int		m_iSelSlogan;		// index of selected slogan
	BOOL	m_bImmediateMode;	// immediate mode checkbox state
	int		m_iTriggerType;		// trigger type radio button index

// Helpers
	bool	InitFontFamilyCombo();
	static void	GetTransTypeNames(CStringArrayEx& aTypeName);
	static void	InitTransTypeCombo(CComboBox& cb, int iSelTransType, const CStringArrayEx& aTypeName);
	void	UpdateTransType(int iDir);
	void	EndSloganEdit();
	void	PushSlogan(CSlogan& src, UINT nColMask);
	static bool	CommitFocusedEdit(HWND hDlgWnd);

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
	afx_msg void OnClickedSloganNew();
	afx_msg void OnClickedTrigger(UINT nID);
	afx_msg void OnClickedGo();
};
