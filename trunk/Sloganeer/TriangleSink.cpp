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
		02		27nov25	radiate from center of glyph's ink box
		03		30dec25	add sort by angle

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

void CTriangleSink::OnStartTrans(CKD2DSizeF szRT)
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

// The triangles returned by ID2D1PathGeometry::Tessellate are in layout box
// relative coordinates. The X coordinate is relative to either the left side
// of the layout box (for LTR languages, X increasing to the right), or its
// right side (for RTL languages, X increasing to the left). The Y coordinate
// is relative to the baseline. Note that points above the baseline will have 
// negative Y values.
//
// The rGlyph argument, in contrast, is the glyph ink box in world coordinates:
// the smallest bounding rectangle that the glyph fits within. This rectangle
// is obtained from the glyph iterator's GetNext method. Note that for a tight
// fit in the Y axis, the CGlyphIter constructor's bTightVertBounds flag must
// be set to true. This is necessary so that the triangles radiate from the
// center of the glyph's ink box, and NOT from the center of its layout box.
// This can be tested using a period, drawn in a serif font that has circular
// periods. The period should disassemble into a ring of triangles precisely
// centered on the middle of the period.
//
// The ptBaselineOrigin argument anchors the layout box in world coordinates.
// It lets us translate the triangles output by the tessellator from layout box
// relative coordinates back into world coordinates. Its X coordinate is either
// the left edge of the layout box (for LTR languages) or its right edge (for
// RTL languages), and its Y coordinate is the baseline of the layout box. The
// X coordinate is obtained from the glyph iterator's GetOrigin accessor. Note
// that GetOrigin must be called BEFORE GetNext, because GetNext advances the
// X coordinate. When the path geometry is rendered, the same ptBaselineOrigin
// should also be used as the translation matrix.
// 
HRESULT CTriangleSink::TessellateGlyph(CKD2DPointF ptBaselineOrigin, const CKD2DRectF& rGlyph, const ID2D1PathGeometry *pPathGeom)
{
	m_rGlyph = rGlyph;
	m_ptGlyphCenterWorld = m_rGlyph.CenterPoint();
	float	fOriginX;
	// compute glyph center in layout box relative coordinates, the same space
    // Tessellate uses for triangle points; X handling differs for LTR vs RTL
	if (m_nRunBidiLevel & 1) { // if odd bidi level, right-to-left language
		fOriginX = rGlyph.right - ptBaselineOrigin.x - rGlyph.Width() / 2;
	} else {	// left-to-right language
		fOriginX = rGlyph.left - ptBaselineOrigin.x + rGlyph.Width() / 2; 
	}
	float fOriginY = rGlyph.bottom - ptBaselineOrigin.y - rGlyph.Height() / 2;
	m_ptGlyphCenterLocal = CKD2DPointF(fOriginX, fOriginY);
	int	nOldTris = m_aTriangle.GetSize();
	HRESULT hr = pPathGeom->Tessellate(D2D1::Matrix3x2F::Identity(), this);
	int	nNewTris = m_aTriangle.GetSize();
	int	nGlyphTris = nNewTris - nOldTris;
	m_aGlyphTriCount.Add(nGlyphTris);
	return hr;
}

void CTriangleSink::SortByAngle()
{
	int	nGlyphs = m_aGlyphTriCount.GetSize();
	int	iStartTri = 0;
	for (int iGlyph = 0; iGlyph < nGlyphs; iGlyph++) {	// for each glyph
		int	nGlyphTris = m_aGlyphTriCount[iGlyph];	// get glyph's triangle count
		if (nGlyphTris) {	// if glyph has triangles
			qsort(&m_aTriangle[iStartTri], nGlyphTris, sizeof(GLYPH_TRIANGLE), 
				SortByAngleCompareFunc);	// sort triangles by angle
			iStartTri += nGlyphTris;	// index to next glyph's first triangle
		}
	}
}

int CTriangleSink::SortByAngleCompareFunc(const void* p1, const void* p2)
{
	// sort into descending order so that angle proceeds clockwise
	GLYPH_TRIANGLE	*pTri1 = (GLYPH_TRIANGLE*)p1;
	GLYPH_TRIANGLE	*pTri2 = (GLYPH_TRIANGLE*)p2;
	if (pTri1->fAngle < pTri2->fAngle)
		return 1;
	if (pTri1->fAngle > pTri2->fAngle)
		return -1;
	return 0;
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
		DPoint	d(ptCentroid - m_ptGlyphCenterLocal);	// convert back to world coords
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
	const CKD2DSizeF& szRT,
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
