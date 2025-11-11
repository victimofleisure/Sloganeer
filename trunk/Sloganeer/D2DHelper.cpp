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
#define _USE_MATH_DEFINES
#include "math.h"

#define DTF(x) static_cast<float>(x)

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
	m_fOriginX = m_ptOrigin.x;
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

void AddEllipse(ID2D1GeometrySink *pSink, D2D1_POINT_2F ptOrigin, D2D1_SIZE_F szRadius)
{
	ASSERT(pSink != NULL);
	CD2DPointF	ptOrg1(ptOrigin.x, ptOrigin.y - szRadius.height);
	CD2DPointF	ptOrg2(ptOrigin.x, ptOrigin.y + szRadius.height);
	pSink->BeginFigure(ptOrg1, D2D1_FIGURE_BEGIN_FILLED);
	D2D1_ARC_SEGMENT	arc1 = {ptOrg2, szRadius,
		0, D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE, D2D1_ARC_SIZE_SMALL};
	D2D1_ARC_SEGMENT	arc2 = {ptOrg1, szRadius,
		0, D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE, D2D1_ARC_SIZE_SMALL};
	pSink->AddArc(&arc1);
	pSink->AddArc(&arc2);
	pSink->EndFigure(D2D1_FIGURE_END_CLOSED);
}

void AddPieWedge(ID2D1GeometrySink *pSink, D2D1_POINT_2F ptOrigin, D2D1_SIZE_F szRadius, float fStartAngle, float fWedgeFrac)
{
	ASSERT(pSink != NULL);
	double	fTheta1 = fStartAngle / 180 * M_PI;
	CD2DPointF	pt1(
		DTF(sin(fTheta1) * szRadius.width + ptOrigin.x), 
		DTF(cos(fTheta1) * szRadius.height + ptOrigin.y));
	double	fTheta2 = fTheta1 + fWedgeFrac * M_PI * 2;
	CD2DPointF	pt2(
		DTF(sin(fTheta2) * szRadius.width + ptOrigin.x), 
		DTF(cos(fTheta2) * szRadius.height + ptOrigin.y));
	//
	// A single AddArc can't draw a circle, so check if the wedge is the
	// entire pie, and if so, handle that by calling AddEllipse instead.
	//
	const double fEpsilon = 0.01;	// tolerance in DIPs
	// if the arc's end points are uncomfortably close together
	if (fabs(pt2.x - pt1.x) < fEpsilon && fabs(pt2.y - pt1.y) < fEpsilon) {
		if (fWedgeFrac > 0.5) // if the wedge is entire pie, draw ellipse
			AddEllipse(pSink, ptOrigin, szRadius);
		return;	// if wedge is ultra-thin, draw nothing
	}
	pSink->BeginFigure(ptOrigin, D2D1_FIGURE_BEGIN_FILLED);
	pSink->AddLine(pt1);
	D2D1_ARC_SIZE	arcSize = fTheta2 - fTheta1 > M_PI ?
		D2D1_ARC_SIZE_LARGE : D2D1_ARC_SIZE_SMALL;
	D2D1_ARC_SEGMENT	arc = {pt2, szRadius, 0,
		D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE, arcSize};
	pSink->AddArc(&arc);
	pSink->EndFigure(D2D1_FIGURE_END_CLOSED);
}
