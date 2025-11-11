// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
		chris korda

		rev		date	comments
		00		11nov25	initial version

*/

#pragma once

#include "stdafx.h"
#include "D2DHelper.h"

CGlyphIter::CGlyphIter(CD2DPointF ptBaselineOrigin, DWRITE_GLYPH_RUN const* pGlyphRun)
	: m_ptOrigin(ptBaselineOrigin)
{
	ASSERT(pGlyphRun != NULL);
	m_pGlyphRun = pGlyphRun;
	DWRITE_FONT_METRICS	fm;
	pGlyphRun->fontFace->GetMetrics(&fm);
	float	fScale = pGlyphRun->fontEmSize / fm.designUnitsPerEm;
	m_fAscent = fm.ascent * fScale;
	m_fDescent = fm.descent * fScale;
	m_iGlyph = 0;
}

bool CGlyphIter::GetNext(UINT& iGlyph, CKD2DRectF& rGlyph)
{
	// assumes a horizontal run
	ASSERT(m_pGlyphRun != NULL);
	iGlyph = m_iGlyph;
	if (iGlyph >= m_pGlyphRun->glyphCount)	// if end of glyphs
		return false;
	float	fAdvance = m_pGlyphRun->glyphAdvances[iGlyph];
	rGlyph.left = m_ptOrigin.x;
	rGlyph.top = m_ptOrigin.y - m_fAscent;
	rGlyph.right = m_ptOrigin.x + fAdvance;
	rGlyph.bottom = m_ptOrigin.y + m_fDescent;
	if (m_pGlyphRun->glyphOffsets != NULL) {	// if offsets specified
		const DWRITE_GLYPH_OFFSET&	goff = m_pGlyphRun->glyphOffsets[iGlyph];
		rGlyph.OffsetRect(goff.advanceOffset, goff.ascenderOffset);
	}
	m_ptOrigin.x += fAdvance;
	m_iGlyph++;
	return true;
}
