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

// ParamsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Sloganeer.h"
#include "SloganeerDlg.h"
#include "ParamsDlg.h"

// CParamsDlg dialog

IMPLEMENT_DYNAMIC(CParamsDlg, CDialog)

CParamsDlg::CParamsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CParamsDlg::IDD, pParent)
{
	m_pAppDlg = NULL;
	m_iSloganPlayMode = 0;
	m_iSelSlogan = -1;
	m_bImmediateMode = false;
	m_iTriggerType = 0;
}

CParamsDlg::~CParamsDlg()
{
}

void CParamsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PARAMS_SLOGAN, m_cbSlogan);
	DDX_Control(pDX, IDC_PARAMS_FONT_NAME, m_cbFontName);
	DDX_Text(pDX, IDC_PARAMS_FONT_SIZE, m_fFontSize);
	DDV_MinMaxFloat(pDX, m_fFontSize, 1, USHORT_MAX);
	DDX_Text(pDX, IDC_PARAMS_FONT_WEIGHT, m_nFontWeight);
	DDV_MinMaxInt(pDX, m_nFontWeight, 1, 999);
	DDX_Check(pDX, IDC_PARAMS_IMMEDIATE_MODE, m_bImmediateMode);
	DDX_Control(pDX, IDC_PARAMS_COLOR_BKGND, m_btnBkgnd);
	DDX_Text(pDX, IDC_PARAMS_COLOR_BKGND_ALPHA, m_clrBkgnd.a);
	DDV_MinMaxFloat(pDX, m_clrBkgnd.a, 0, 1);
	DDX_Control(pDX, IDC_PARAMS_COLOR_DRAW, m_btnDraw);
	DDX_Text(pDX, IDC_PARAMS_COLOR_DRAW_ALPHA, m_clrDraw.a);
	DDV_MinMaxFloat(pDX, m_clrDraw.a, 0, 1);
	DDX_Radio(pDX, IDC_PARAMS_PLAY_MODE_0, m_iSloganPlayMode);
	DDX_Text(pDX, IDC_PARAMS_DUR_IN, m_fInTransDur);
	DDV_MinMaxFloat(pDX, m_fInTransDur, 0, float(USHORT_MAX));
	DDX_Text(pDX, IDC_PARAMS_DUR_HOLD, m_fHoldDur);
	DDV_MinMaxFloat(pDX, m_fHoldDur, 0, float(USHORT_MAX));
	DDX_Text(pDX, IDC_PARAMS_DUR_OUT, m_fOutTransDur);
	DDV_MinMaxFloat(pDX, m_fOutTransDur, 0, float(USHORT_MAX));
	DDX_Text(pDX, IDC_PARAMS_DUR_PAUSE, m_fPauseDur);
	DDV_MinMaxFloat(pDX, m_fPauseDur, 0, float(USHORT_MAX));
	DDX_Control(pDX, IDC_PARAMS_TRANS_TYPE_IN, m_cbTransType[TD_INCOMING]);
	DDX_Control(pDX, IDC_PARAMS_TRANS_TYPE_OUT, m_cbTransType[TD_OUTGOING]);
	DDX_Radio(pDX, IDC_PARAMS_TRIGGER_TYPE_0, m_iTriggerType);
}

BOOL CParamsDlg::OnInitDialog()
{
	// get pointer to app dialog
	CSloganeerDlg*	pParentDlg = STATIC_DOWNCAST(CSloganeerDlg, theApp.m_pMainWnd);
	ASSERT(pParentDlg != NULL);
	m_pAppDlg = pParentDlg;
	// do init that must precede child control creation
	const CSloganParams& params = pParentDlg->m_sd.GetParams();
	CSlogan::operator=(pParentDlg->m_initSlogan);
	m_iSloganPlayMode = params.m_iSloganPlayMode;
	// create child controls
	CDialog::OnInitDialog();
	// do init that must follow child control creation
	int	nSlogans = params.m_aSlogan.GetSize();
	for (int iSlogan = 0; iSlogan < nSlogans; iSlogan++) {	// for each slogan
		CString	sSlogan(params.m_aSlogan[iSlogan].m_sText);
		UnescapeChars(sSlogan);
		m_cbSlogan.AddString(sSlogan);	// add slogan to combo box
	}
	if (nSlogans)
		m_cbSlogan.SetCurSel(0);
	InitFontFamilyCombo();
	m_btnBkgnd.SetColor(ColorFBGR(m_clrBkgnd));
	m_btnDraw.SetColor(ColorFBGR(m_clrDraw));
	CStringArrayEx	aTransTypeName;
	GetTransTypeNames(aTransTypeName);
	for (int iDir = 0; iDir < TRANS_DIRS; iDir++) {
		InitTransTypeCombo(m_cbTransType[iDir], m_aTransType[iDir], aTransTypeName);
	}
	pParentDlg->m_sd.SetCustomSlogans(true);
	return TRUE;  // return TRUE unless you set the focus to a control
}

bool CParamsDlg::InitFontFamilyCombo()
{
	CD2DFontCollection	collFont;
	if (!collFont.Create())
		return false;
	int	nFams = collFont.GetFamilyCount();
	int	nFamChars = 0;
	// compute total characters needed for family names
	for (int iFam = 0; iFam < nFams; iFam++) {
		nFamChars += collFont.GetFamilyName(iFam).GetLength() + 1;	// include terminator
	}
	// preallocate font combo box storage in items and bytes
	m_cbFontName.InitStorage(nFams, nFamChars * sizeof(TCHAR));
	m_cbFontName.SetRedraw(false);	// disable redraw for faster adding
	for (int iFam = 0; iFam < nFams; iFam++) {	// for each family
		m_cbFontName.AddString(collFont.GetFamilyName(iFam));
	}
	// find selected item in combo box; account for none item
	int	iSel = m_cbFontName.FindStringExact(-1, m_sFontName);
	if (iSel >= 0)	// if item found
		m_cbFontName.SetCurSel(iSel);	// set current selection
	m_cbFontName.SetRedraw();	// reenable redraw
	return true;
}

void CParamsDlg::GetTransTypeNames(CStringArrayEx& aTypeName)
{
	static const int aTransTypeNameID[] = {
		IDS_NONE,	// none item comes first
		#define TRANSTYPEDEF(code, name) IDS_TT_##name,
		#include "ParamDef.h" // generate code
	};
	aTypeName.SetSize(TRANS_TYPES + 1);	// one extra for none item
	// for each transition type, offset by one for none item
	for (int iType = 0; iType <= TRANS_TYPES; iType++) {
		aTypeName[iType].LoadString(aTransTypeNameID[iType]);
	}
}

void CParamsDlg::InitTransTypeCombo(CComboBox& cb, int iSelTransType, const CStringArrayEx& aTypeName)
{
	// selected transition type is a zero-based index or -1 for none
	cb.SetRedraw(false);	// disable redraw for faster adding
	// for each transition type, offset by one for none item
	for (int iType = 0; iType <= TRANS_TYPES; iType++) {
		int	iPos = cb.AddString(aTypeName[iType]);
		cb.SetItemData(iPos, iType - 1);	// account for none item
	}
	// find selected item in combo box; account for none item
	int	iSel = cb.FindStringExact(-1, aTypeName[iSelTransType + 1]);
	if (iSel >= 0)	// if item found
		cb.SetCurSel(iSel);	// set current selection
	cb.SetRedraw();	// reenable redraw
}

void CParamsDlg::EndSloganEdit()
{
	int	iSlogan = m_iSelSlogan;
	if (iSlogan >= 0) {	// if selected slogan is valid
		CString	sEditText;	// retrieve slogan text
		m_cbSlogan.GetWindowText(sEditText);
		CString	sLBText;	// retrieve list box text for selected item
		m_cbSlogan.GetLBText(iSlogan, sLBText);
		if (sEditText != sLBText) {	// if edited and list box text differ
			m_cbSlogan.DeleteString(iSlogan);	// delete item from list
			m_cbSlogan.InsertString(iSlogan, sEditText);	// insert edited item
			m_cbSlogan.SetCurSel(iSlogan);	// set current selection to item
			EscapeChars(sEditText);	// handle escape sequences
			m_pAppDlg->m_sd.SetSloganText(iSlogan, sEditText);	// update view
		}
	}
}

void CParamsDlg::PushSlogan(CSlogan& src, UINT nColMask)
{
	src.m_nCustomColMask = nColMask;
	m_pAppDlg->m_sd.SetSlogan(src);
}

bool CParamsDlg::CommitFocusedEdit(HWND hDlgWnd)
{
	HWND	hFocusWnd = ::GetFocus();	// get focus window
	if (::IsChild(hDlgWnd, hFocusWnd)) {	// if focus window is dialog's child
		// if focus window is an edit control
		TCHAR	szClassName[32];
		if (GetClassName(hFocusWnd, szClassName, 6) 
		&& _tcsicmp(szClassName, _T("Edit")) == 0) {
			// get focus window's control ID
			UINT	id = ::GetDlgCtrlID(hFocusWnd);
			// run the dialog's EN_KILLFOCUS handler for that control ID
			::SendMessage(hDlgWnd, WM_COMMAND, 
				MAKEWPARAM(id, EN_KILLFOCUS), (LPARAM)hFocusWnd);
			// select all the text in the edit control
			::SendMessage(hFocusWnd, EM_SETSEL, 0, -1);
			return true;
		}
	}
	return false;
}

void CParamsDlg::SetSlogans(const CSloganArray& aSlogan)
{
	if (aSlogan.IsEmpty())	// if no slogans
		return;	// user error
	if (!m_hWnd)	// if dialog isn't created
		return;	// can't proceed
	m_cbSlogan.ResetContent();	// empty combo box
	for (int iSlogan = 0; iSlogan < aSlogan.GetSize(); iSlogan++) {	// for each slogan
		m_cbSlogan.AddString(aSlogan[iSlogan].m_sText);	// add slogan text to combo box
	}
	m_cbSlogan.SetCurSel(0);	// select first slogan
}

BEGIN_MESSAGE_MAP(CParamsDlg, CDialog)
	ON_WM_CLOSE()
	ON_CBN_SELENDOK(IDC_PARAMS_SLOGAN, OnSelendokSlogan)
	ON_CBN_KILLFOCUS(IDC_PARAMS_SLOGAN, OnKillfocusSlogan)
	ON_CBN_SELENDOK(IDC_PARAMS_FONT_NAME, OnSelendokFontName)
	ON_EN_KILLFOCUS(IDC_PARAMS_FONT_SIZE, OnKillfocusFontSize)
	ON_EN_KILLFOCUS(IDC_PARAMS_FONT_WEIGHT, OnKillfocusFontWeight)
	ON_BN_CLICKED(IDC_PARAMS_COLOR_BKGND, OnClickedColorBkgnd)
	ON_EN_KILLFOCUS(IDC_PARAMS_COLOR_BKGND_ALPHA, OnKillfocusColorBkgndAlpha)
	ON_BN_CLICKED(IDC_PARAMS_COLOR_DRAW, OnClickedColorDraw)
	ON_EN_KILLFOCUS(IDC_PARAMS_COLOR_DRAW_ALPHA, OnKillfocusColorDrawAlpha)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_PARAMS_PLAY_MODE_0, IDC_PARAMS_PLAY_MODE_2, OnClickedPlayMode)
	ON_EN_KILLFOCUS(IDC_PARAMS_DUR_IN, OnKillfocusDurIn)
	ON_EN_KILLFOCUS(IDC_PARAMS_DUR_HOLD, OnKillfocusDurHold)
	ON_EN_KILLFOCUS(IDC_PARAMS_DUR_OUT, OnKillfocusDurOut)
	ON_EN_KILLFOCUS(IDC_PARAMS_DUR_PAUSE, OnKillfocusDurPause)
	ON_CBN_SELENDOK(IDC_PARAMS_TRANS_TYPE_IN, OnSelendokTransTypeIn)
	ON_CBN_SELENDOK(IDC_PARAMS_TRANS_TYPE_OUT, OnSelendokTransTypeOut)
	ON_BN_CLICKED(IDC_PARAMS_IMMEDIATE_MODE, OnClickedImmediateMode)
	ON_BN_CLICKED(IDC_PARAMS_SLOGAN_NEW, OnClickedSloganNew)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_PARAMS_TRIGGER_TYPE_0, IDC_PARAMS_TRIGGER_TYPE_1, OnClickedTrigger)
	ON_BN_CLICKED(IDC_PARAMS_TRIGGER_GO, OnClickedGo)
END_MESSAGE_MAP()

// CParamsDlg message handlers

void CParamsDlg::OnOK()
{
}

void CParamsDlg::OnCancel()
{
}

void CParamsDlg::OnClose()
{
	EndDialog(IDOK);
}

BOOL CParamsDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN) {
		if (m_pAppDlg->HandleKey(static_cast<UINT>(pMsg->wParam)))
			return true;
		if (pMsg->wParam == VK_RETURN) {	// if return key
			if (CommitFocusedEdit(m_hWnd))
				return true;
		}
	}
	return CDialog::PreTranslateMessage(pMsg);
}

// push parameter if its value changed
#define PUSH_PARAM(mbr, tag) \
	if (mbr != slogan.mbr) { \
		slogan.mbr = mbr; \
		PushSlogan(slogan, tag); \
	}

// save and validate dialog data; then push parameter if its value changed
#define UPDATE_PARAM(mbr, tag) \
	CSlogan	slogan(*this); \
	if (UpdateData()) { \
		PUSH_PARAM(mbr, tag); \
	}

// retrieve BGR color from color button; then push D2D color unconditionally
#define PUSH_COLOR(btn, mbr, tag) \
	slogan.mbr = BGRColorF(btn.GetColor(), mbr.a); \
	PushSlogan(slogan, tag);

// save and validate dialog data; then push D2D color if its alpha changed
#define UPDATE_ALPHA(btn, mbr, tag) \
	CSlogan	slogan(*this); \
	if (UpdateData() && mbr.a != slogan.mbr.a) { \
		PUSH_COLOR(btn, mbr, tag); \
	}

void CParamsDlg::OnSelendokSlogan()
{
	int	iSlogan = m_cbSlogan.GetCurSel();
	if (iSlogan >= 0) {	// if valid item selection
		m_pAppDlg->m_sd.SelectSlogan(iSlogan);
	}
	m_iSelSlogan = iSlogan;
}

void CParamsDlg::OnKillfocusSlogan()
{
	EndSloganEdit();
}

void CParamsDlg::OnSelendokFontName()
{
	int	iSel = m_cbFontName.GetCurSel();
	if (iSel >= 0) {	// if valid selection
		CSlogan	slogan(*this);
		m_cbFontName.GetLBText(iSel, m_sFontName);	// get combo box string
		PUSH_PARAM(m_sFontName, CBM_fontname);
	}
}

void CParamsDlg::OnKillfocusFontSize()
{
	UPDATE_PARAM(m_fFontSize, CBM_fontsize);
}

void CParamsDlg::OnKillfocusFontWeight()
{
	UPDATE_PARAM(m_nFontWeight, CBM_fontwt);
}

void CParamsDlg::OnClickedColorBkgnd()
{
	CSlogan	slogan;
	PUSH_COLOR(m_btnBkgnd, m_clrBkgnd, CBM_bgclr);
}

void CParamsDlg::OnKillfocusColorBkgndAlpha()
{
	UPDATE_ALPHA(m_btnBkgnd, m_clrBkgnd, CBM_bgclr);
}

void CParamsDlg::OnClickedColorDraw()
{
	CSlogan	slogan;
	PUSH_COLOR(m_btnDraw, m_clrDraw, CBM_drawclr);
}

void CParamsDlg::OnKillfocusColorDrawAlpha()
{
	UPDATE_ALPHA(m_btnDraw, m_clrDraw, CBM_drawclr);
}

void CParamsDlg::OnClickedPlayMode(UINT nID)
{
	int	iMode = nID - IDC_PARAMS_PLAY_MODE_0;
	ASSERT(iMode >= 0 && iMode < CSloganParams::SLOGAN_PLAY_MODES);
	m_pAppDlg->m_sd.SetSloganPlayMode(iMode);
}

void CParamsDlg::OnKillfocusDurIn()
{
	UPDATE_PARAM(m_fInTransDur, CBM_transdur);
}

void CParamsDlg::OnKillfocusDurHold()
{
	UPDATE_PARAM(m_fHoldDur, CBM_holddur);
}

void CParamsDlg::OnKillfocusDurOut()
{
	UPDATE_PARAM(m_fOutTransDur, CBM_outdur);
}

void CParamsDlg::OnKillfocusDurPause()
{
	UPDATE_PARAM(m_fPauseDur, CBM_pausedur);
}

void CParamsDlg::UpdateTransType(int iDir)
{
	int	iSel = m_cbTransType[iDir].GetCurSel();
	if (iSel >= 0) {	// if valid selection
		CSlogan	slogan(*this);
		m_aTransType[iDir] = static_cast<int>(m_cbTransType[iDir].GetItemData(iSel));
		PUSH_PARAM(m_aTransType[iDir], iDir == TD_INCOMING ? CBM_intrans : CBM_outtrans);
	}
}

void CParamsDlg::OnSelendokTransTypeIn()
{
	UpdateTransType(TD_INCOMING);
}

void CParamsDlg::OnSelendokTransTypeOut()
{
	UpdateTransType(TD_OUTGOING);
}

void CParamsDlg::OnClickedImmediateMode()
{
	if (UpdateData()) {
		m_pAppDlg->m_sd.SetImmediateMode(m_bImmediateMode != 0);
	}
}

void CParamsDlg::OnClickedSloganNew()
{
	EndSloganEdit();
	int	iItem = m_cbSlogan.AddString(_T(""));	// add empty string to combo box
	if (iItem >= 0) {	// if string was added
		CSlogan	slogan(m_pAppDlg->m_sd.GetParams());	// use current slogan defaults
		slogan.m_sText.Empty();	// except text is empty
		m_pAppDlg->m_sd.InsertSlogan(iItem, slogan);	// insert slogan
		m_pAppDlg->m_sd.SelectSlogan(iItem);	// select slogan
		m_cbSlogan.SetCurSel(iItem);	// select combo box item
		m_iSelSlogan = iItem;	// update selected slogan index
		m_cbSlogan.SetFocus();	// focus combo box edit control for convenience
	}
}

void CParamsDlg::OnClickedTrigger(UINT nID)
{
	int	iTrigger = nID - IDC_PARAMS_TRIGGER_TYPE_0;
	ASSERT(iTrigger >= 0 && iTrigger < TRIGGER_TYPES);
	m_pAppDlg->m_sd.SetTriggerType(iTrigger != 0);
}

void CParamsDlg::OnClickedGo()
{
	EndSloganEdit();
	m_pAppDlg->m_sd.TriggerGo();
}
