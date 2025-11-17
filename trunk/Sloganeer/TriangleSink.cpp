// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      16nov25	initial version

*/

#include "stdafx.h"
#include "TriangleSink.h"
#include "DPoint.h"
#include <limits>
#include <float.h>
#include <math.h>

CTriangleSink::CTriangleSink()
{
	m_iGlyphFirstTri = 0;
	m_iCurGlyph = 0;
	m_fTravel = 0;
}

void CTriangleSink::OnStartTrans(CD2DSizeF szRT)
{
	m_szRT = szRT;
	m_aTriangle.FastRemoveAll();
	m_aGlyphTriCount.FastRemoveAll();
	m_fTravel = 0;
}

void CTriangleSink::OnDraw()
{
	m_iGlyphFirstTri = 0;
	m_iCurGlyph = 0;
}

HRESULT CTriangleSink::TessellateGlyph(CD2DPointF ptBaselineOrigin, const CKD2DRectF& rGlyph, const ID2D1PathGeometry *pPathGeom)
{
	m_ptBaselineOrigin = ptBaselineOrigin;
	m_rGlyph = rGlyph;
	m_ptGlyphCenterWorld = m_rGlyph.CenterPoint();
	float	fBaselineOffset = rGlyph.bottom - ptBaselineOrigin.y;
	m_ptGlyphCenterLocal = CD2DPointF(rGlyph.Width() / 2, fBaselineOffset - rGlyph.Height() / 2);
	int	nOldTris = m_aTriangle.GetSize();
	HRESULT hr = pPathGeom->Tessellate(D2D1::Matrix3x2F::Identity(), this);
	int	nNewTris = m_aTriangle.GetSize();
	int	nGlyphTris = nNewTris - nOldTris;
	m_aGlyphTriCount.Add(nGlyphTris);
	return hr;
}

void CTriangleSink::GetNextGlyph(int& iStartTri, int& iEndTri)
{
	iStartTri = m_iGlyphFirstTri;
	m_iGlyphFirstTri += m_aGlyphTriCount[m_iCurGlyph];
	iEndTri = m_iGlyphFirstTri;
	m_iCurGlyph++;
}

HRESULT CTriangleSink::QueryInterface(REFIID riid, void** ppv)
{
	if (!ppv)
		return E_INVALIDARG;
	if (riid == __uuidof(IUnknown) || riid == __uuidof(ID2D1TessellationSink)) {
		*ppv = static_cast<ID2D1TessellationSink*>(this);
		AddRef();
		return S_OK;
	}
	*ppv = NULL;
	return E_NOINTERFACE;
}

void CTriangleSink::AddTriangles(const D2D1_TRIANGLE* triangles, UINT32 trianglesCount)
{
	for (UINT iTri = 0; iTri < trianglesCount; iTri++) {	// for each triangle
		const D2D1_TRIANGLE&	t = triangles[iTri];
		DPoint	p1(t.point1);
		DPoint	p2(t.point2);
		DPoint	p3(t.point3);
		DPoint	ptCentroid((p1 + p2 + p3) / 3);
		DPoint	d(ptCentroid - m_ptGlyphCenterLocal);
		int	iGlyphTri = m_aTriangle.GetSize();
		m_aTriangle.FastSetSize(iGlyphTri + 1);	// grow array if needed
		GLYPH_TRIANGLE&	gt = m_aTriangle[iGlyphTri];
		gt.D2D1_TRIANGLE::operator=(t);	// copy triangle into array
		gt.fAngle = static_cast<float>(atan2(d.x, d.y));	// set triangle's angle
		double	fLen = sqrt(d.x * d.x + d.y * d.y);
		const double fEpsilon = 1e-5;
		if (fLen < fEpsilon)
			continue;
		DPoint	ptUV(d / fLen);	// compute unit vector
		float	triMax = 0;
		float	fDist = CalcTravel(m_szRT, m_ptGlyphCenterWorld, ptUV);
		triMax = max(triMax, fDist);
		m_fTravel = max(m_fTravel, triMax);
	}
}

inline bool my_isfinite(double x)
{
    return _finite(x) && !_isnan(x);
}

// this method was written by GPT5
float CTriangleSink::CalcTravel(D2D1_SIZE_F rtSize, D2D1_POINT_2F glyphCenterWorld, D2D1_POINT_2F uv)
{
	// Recenter so (0,0) is the center of the window.
	const float halfW = rtSize.width  * 0.5f;
	const float halfH = rtSize.height * 0.5f;

	// Recenter so rect is [-halfW, +halfW] x [-halfH, +halfH]
	float cx = glyphCenterWorld.x - halfW;
	float cy = glyphCenterWorld.y - halfH;

	// If glyph center starts outside the rect, it's already "offscreen"
	// in the assembled pose. Let it contribute zero travel so it doesn't
	// blow up the global max.
	if (cx <= -halfW || cx >= halfW ||
		cy <= -halfH || cy >= halfH) {
		return 0.0f;
	}

	float tMin = std::numeric_limits<float>::infinity();

	// Right side: x = +halfW
	if (uv.x > 0.0f) {
		float t = (halfW - cx) / uv.x;
		if (t > 0.0f && t < tMin) tMin = t;
	}
	// Left side: x = -halfW
	if (uv.x < 0.0f) {
		float t = (-halfW - cx) / uv.x;
		if (t > 0.0f && t < tMin) tMin = t;
	}
	// Bottom: y = +halfH
	if (uv.y > 0.0f) {
		float t = (halfH - cy) / uv.y;
		if (t > 0.0f && t < tMin) tMin = t;
	}
	// Top: y = -halfH
	if (uv.y < 0.0f) {
		float t = (-halfH - cy) / uv.y;
		if (t > 0.0f && t < tMin) tMin = t;
	}

	if (!my_isfinite(tMin))
		return 0.0f; // purely defensive: direction is pointing "inside" forever?

	return tMin;
}
