// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      16nov25	initial version
		01		24nov25	add bidi level for RTL languages

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
	m_nRunBidiLevel = 0;
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
	if (m_nRunBidiLevel & 1) // if odd bidi level, right-to-left language
		m_ptGlyphCenterLocal.x = -m_ptGlyphCenterLocal.x;	// flip x coord around x-axis
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

		// remainder of this method is for CalcTravel
		double fLen = d.Length();
		const double fEpsilon = 1e-5;
		if (fLen < fEpsilon)
			continue;   // direction undefined, skip this tri

		DPoint ptUV = d / fLen; // unit vector for this triangle

		// Compute a conservative radius for the triangle around its centroid
		double r1 = (p1 - ptCentroid).Length();
		double r2 = (p2 - ptCentroid).Length();
		double r3 = (p3 - ptCentroid).Length();
		float triRadius = static_cast<float>(max(r1, max(r2, r3)));

		// Centroid in world coords:
		// assuming glyph local origin is m_ptGlyphCenterLocal and
		// m_ptGlyphCenterWorld is the world position of that origin.
		DPoint ptCentroidWorld(
			m_ptGlyphCenterWorld.x + (ptCentroid.x - m_ptGlyphCenterLocal.x),
			m_ptGlyphCenterWorld.y + (ptCentroid.y - m_ptGlyphCenterLocal.y)
		);

		float fDist = CalcTravel(m_szRT, ptCentroidWorld, ptUV, triRadius);

		m_fTravel = max(m_fTravel, fDist);
	}
}

// this method was written by GPT5
float CTriangleSink::CalcTravel(
	const CD2DSizeF& szRT,
	const DPoint& ptWorld,   // point in world coords
	const DPoint& dir,       // direction (doesn't need to be normalized)
	float triRadius)         // radius of triangle around ptWorld
{
	const double eps = 1e-6;

	// Normalize direction
	double dx = dir.x;
	double dy = dir.y;
	double len = sqrt(dx * dx + dy * dy);
	if (len < eps)
		return 0.0f; // degenerate direction

	dx /= len;
	dy /= len;

	// Center the RT: we want bounds [-halfW, +halfW] x [-halfH, +halfH]
	double halfW = szRT.width  * 0.5;
	double halfH = szRT.height * 0.5;

	// Express origin in centered coordinates
	double ox = ptWorld.x - halfW;
	double oy = ptWorld.y - halfH;

	// Slab-based ray vs AABB clip
	double tMin = 0.0;
	double tMax = std::numeric_limits<double>::infinity();

	auto clipAxis = [&](double o, double d, double minB, double maxB) -> bool {
		if (fabs(d) < eps) {
			// Ray is parallel to this axis slab: must already be within bounds
			if (o < minB || o > maxB)
				return false;   // no intersection at all
			return true;        // no change to tMin/tMax
		}

		double t1 = (minB - o) / d;
		double t2 = (maxB - o) / d;
		if (t1 > t2)
			Swap(t1, t2);

		// Intersect this [t1, t2] with current [tMin, tMax]
		if (t2 < tMin || t1 > tMax)
			return false;   // disjoint

		if (t1 > tMin) tMin = t1;
		if (t2 < tMax) tMax = t2;
		return true;
	};

	// X slab: [-halfW, +halfW]
	if (!clipAxis(ox, dx, -halfW, +halfW))
		return 0.0f;

	// Y slab: [-halfH, +halfH]
	if (!clipAxis(oy, dy, -halfH, +halfH))
		return 0.0f;

	// After clipping, [tMin, tMax] is the interval where the point is inside the RT.
	// Because we initialized tMin=0, this is already ray-clipped to t>=0.
	if (tMax < 0.0)
		return 0.0f;  // intersection is entirely behind the origin

	// The last contact with the RT is at tMax.
	float tExit = static_cast<float>(tMax);

	// Inflate by triangle radius so the *whole* triangle is out,
	// not just the representative point.
	return max(0.0f, tExit + triRadius);
}
