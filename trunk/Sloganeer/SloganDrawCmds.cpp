// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
		00		11dec25	initial version

*/

#include "stdafx.h"
#include "Sloganeer.h"
#include "SloganDraw.h"

bool CSloganDraw::PushDrawCommand(const CRenderCmd& cmd)
{
	if (!PushCommand(cmd))	// queue command to render thread
		return false;	// retries aren't currently supported
	if (m_bIsImmediateMode)	// if commands should have immediate effect
		m_evtIdleWake.Set();	// render thread could be blocked in idle wait, so wake it
	return true;
}

void CSloganDraw::SetSlogan(const CSlogan& slogan)
{
	CRenderCmd cmd(RC_SET_SLOGAN);
	cmd.m_prop.byref = new CSlogan(slogan);	// recipient is responsible for deletion
	PushDrawCommand(cmd);
}

void CSloganDraw::InsertSlogan(int iSlogan, const CSlogan& slogan)
{
	CRenderCmd cmd(RC_INSERT_SLOGAN, iSlogan);
	cmd.m_prop.byref = new CSlogan(slogan);	// recipient is responsible for deletion
	PushDrawCommand(cmd);
}

void CSloganDraw::SetSloganText(int iSlogan, CString sSlogan)
{
	CRenderCmd cmd(RC_SET_SLOGAN_TEXT, iSlogan);
	cmd.m_prop.byref = SafeStrDup(sSlogan);	// recipient is responsible for deletion
	PushDrawCommand(cmd);
}

void CSloganDraw::SelectSlogan(int iSlogan)
{
	PushDrawCommand(CRenderCmd(RC_SELECT_SLOGAN, iSlogan));
}

void CSloganDraw::SetSloganPlayMode(int iPlayMode)
{
	PushDrawCommand(CRenderCmd(RC_SET_SLOGAN_PLAY_MODE, iPlayMode));
}

void CSloganDraw::SetCustomSlogans(bool bEnable)
{
	// render thread only reads this boolean; command isn't queued
	m_bCustomSlogans = bEnable;	// has immediate effect
}

void CSloganDraw::SetImmediateMode(bool bEnable)
{
	// render thread only reads this boolean; command isn't queued
	m_bIsImmediateMode = bEnable;	// has immediate effect
}

void CSloganDraw::UpdateCurrentSloganText()
{
	if (m_iSlogan >= 0) {	// if current slogan index is valid
		if (IsIdle())	// if idling, assume we're blocked in wait
			m_evtIdleWake.Set();	// force render to make update visible
		m_sSlogan = m_aSlogan[m_iSlogan].m_sText;	// update current slogan
		OnResize();	// update layout and reset various dependent states
	}
}

void CSloganDraw::OnRenderCommand(const CRenderCmd& cmd)
{
//	printf("OnRenderCommand %d %d %llx\n", cmd.m_nCmd, cmd.m_nParam, cmd.m_prop);
	switch (cmd.m_nCmd) {
	case RC_SET_SLOGAN:
		{
			ASSERT(cmd.m_prop.byref != NULL);	// pointer must not be null
			// assume byref points to a CSlogan dynamically allocated via new
			// pSlogan will be deleted automatically when we exit this scope
			CAutoPtr<CSlogan>	pSlogan(static_cast<CSlogan*>(cmd.m_prop.byref));
			UINT	nUpdateColMask = pSlogan->m_nCustomColMask;
			m_aSlogan.UpdateColumns(*pSlogan, nUpdateColMask);
			if (m_bIsImmediateMode) {	// if immediate effect desired
				if (IsIdle())	// if idling, assume we're blocked in wait
					m_evtIdleWake.Set();	// force render to make update visible
				OnCustomSlogan();	// applies font and color updates
				if (nUpdateColMask & CGBM_DURATION) {	// if duration updated
					switch (m_iState) {	// duration updates are state-dependent
					case ST_TRANS_IN:
						if (nUpdateColMask & CBM_transdur)	// if incoming duration
							m_fTransDur = m_fInTransDur;
						break;
					case ST_HOLD:
						if (nUpdateColMask & CBM_holddur)	// if hold duration
							m_nIdleEndTime = m_nIdleStartTime + m_nHoldDur;
						break;
					case ST_TRANS_OUT:
						if (nUpdateColMask & CBM_outdur)	// if outgoing duration
							m_fTransDur = m_fOutTransDur;
						break;
					case ST_PAUSE:
						if (nUpdateColMask & CBM_pausedur)	// if pause duration
							m_nIdleEndTime = m_nIdleStartTime + m_nPauseDur;
						break;
					}
				}
				if (nUpdateColMask && CGBM_TRANSITION) {	// if transition type updated
					if (!IsIdle()) {	// if currently in transition state
						double	fRemainDur = m_fTransDur - m_timerTrans.Elapsed();
						if (fRemainDur > 0) {	// if transition has duration remaining
							double	fStartTime = m_timerTrans.GetStart();	// save start time
							StartTrans(m_iState, static_cast<float>(fRemainDur));	// restart transition
							m_timerTrans.SetStart(fStartTime);	// restore previous start time
						}
					}
				}
			}
			if (nUpdateColMask & CGBM_FONT)	// if melt stroke widths are affected
				LaunchMeltWorker();	// launch melt probe worker
		}
		break;
	case RC_INSERT_SLOGAN:
		{
			int	iSlogan = cmd.m_nParam;
			ASSERT(iSlogan >= 0 && iSlogan <= m_aSlogan.GetSize());	// check insert position
			ASSERT(cmd.m_prop.byref != NULL);	// pointer must not be null
			CAutoPtr<CSlogan>	pSlogan(static_cast<CSlogan*>(cmd.m_prop.byref));
			// we need to insert an element into the melt stroke array, but the melt
			// worker thread writes to that array, risking a race or access violation
			m_thrMeltWorker.Destroy();	// wait for melt worker to exit
			m_aSlogan.InsertAt(iSlogan, *pSlogan);	// update slogan array
			float	fStroke = 0;	// new slogan's melt stroke is initially unknown
			m_aMeltStroke.InsertAt(iSlogan, fStroke);	// update melt stroke array
			if (iSlogan <= m_iSlogan)	// if insertion is at or below current slogan
				m_iSlogan++;	// bump current slogan index to account for insertion
			if (!pSlogan->m_sText.IsEmpty())	// if inserted slogan's text isn't empty
				LaunchMeltWorker(iSlogan);	// probe this slogan only
		}
		break;
	case RC_SET_SLOGAN_TEXT:
		{
			int	iSlogan = cmd.m_nParam;
			ASSERT(iSlogan >= 0 && iSlogan < m_aSlogan.GetSize());	// check index range
			ASSERT(cmd.m_prop.byref != NULL);	// pointer must not be null
			// assume byref points to a string dynamically allocated via new
			LPCTSTR	pszString = static_cast<LPCTSTR>(cmd.m_prop.byref);
			m_aSlogan[iSlogan].m_sText = pszString;
			delete pszString;	// we're responsible for deletion
			if (m_bIsImmediateMode) {	// if immediate effect desired
				UpdateCurrentSloganText();
			}
			LaunchMeltWorker(iSlogan);	// probe this slogan only
		}
		break;
	case RC_SELECT_SLOGAN:
		{
			int	iSlogan = cmd.m_nParam;
			ASSERT(iSlogan >= 0 && iSlogan < m_aSlogan.GetSize());	// check index range
			m_iSlogan = iSlogan;
			if (m_bIsImmediateMode) {	// if immediate effect desired
				UpdateCurrentSloganText();
			} else {	// apply change to next slogan
				if (m_iSloganPlayMode == SPM_SEQUENTIAL)	// if sequential
					m_iSlogan--;	// StartSlogan increments index before using it
			}
		}
		break;
	case RC_SET_SLOGAN_PLAY_MODE:
		{
			int	iPlayMode = cmd.m_nParam;
			ASSERT(iPlayMode >= 0 && iPlayMode < SLOGAN_PLAY_MODES);	// check index range
			m_iSloganPlayMode = iPlayMode;
		}
		break;
	default:
		NODEFAULTCASE;	// unknown command
	}
}
