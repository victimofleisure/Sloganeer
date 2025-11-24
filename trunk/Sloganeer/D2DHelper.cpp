// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
		chris korda

		rev		date	comments
		00		11nov25	initial version
		01		23nov25	fix incorrect x-axis bounds in glyph iterator
		02		24nov25	fix glyph iterator handling of RTL languages

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
	DWRITE_FONT_METRICS	fontMetrics;	// font metrics for run
	pGlyphRun->fontFace->GetMetrics(&fontMetrics);
	m_fEmScale = pGlyphRun->fontEmSize / fontMetrics.designUnitsPerEm;
	m_fAscent = fontMetrics.ascent * m_fEmScale;
	m_fDescent = fontMetrics.descent * m_fEmScale;
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
	UINT16	nGlyphIndex = m_pGlyphRun->glyphIndices[iGlyph];
	float	fGlyphAdvance = m_pGlyphRun->glyphAdvances[iGlyph];
	DWRITE_GLYPH_METRICS	gm;
	m_pGlyphRun->fontFace->GetDesignGlyphMetrics(&nGlyphIndex, 1, &gm);
	float	fAdvanceWidth = gm.advanceWidth * m_fEmScale;	// different from fGlyphAdvance
	float	fLeftBearing = gm.leftSideBearing * m_fEmScale;
	float	fRightBearing = gm.rightSideBearing * m_fEmScale;
	bool	bRTL = m_pGlyphRun->bidiLevel & 1;	// odd level indicates right-to-left
	// horizontal ink extents (black box) in run direction
	if (bRTL) {	// if right-to-left
		rGlyph.left = m_ptOrigin.x - fAdvanceWidth + fLeftBearing;
		rGlyph.right = m_ptOrigin.x - fRightBearing;
	} else {	// left-to-right
		rGlyph.left = m_ptOrigin.x + fLeftBearing;
		rGlyph.right = m_ptOrigin.x + fAdvanceWidth - fRightBearing;
	}
	rGlyph.top = m_ptOrigin.y - m_fAscent;
	rGlyph.bottom = m_ptOrigin.y + m_fDescent;
	if (m_pGlyphRun->glyphOffsets) {	// if offsets specified
		const DWRITE_GLYPH_OFFSET& goff = m_pGlyphRun->glyphOffsets[iGlyph];
		// advanceOffset is along the run direction, ascenderOffset is along Y
		// we're in screen coords, so if right-to-left, flip advanceOffset
		float	fAdvOffset = bRTL ? -goff.advanceOffset : goff.advanceOffset;
		rGlyph.OffsetRect(fAdvOffset, goff.ascenderOffset);
	}
	if (bRTL) {	// if right-to-left
		m_ptOrigin.x -= fGlyphAdvance;	// negative advance
	} else {	// left-to-right
		m_ptOrigin.x += fGlyphAdvance;	// positive advance
	}
	m_iGlyph++;	// next glyph
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
