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
		02		27nov25	remove baseline origin point member

*/

#pragma once

#include "D2DHelper.h"

class DPoint;

class CTriangleSink : public ID2D1TessellationSink {
public:
// Construction
	CTriangleSink();

// Types
	struct GLYPH_TRIANGLE : D2D1_TRIANGLE {
		float	fAngle;	// angle of line between centroid and glyph center, in radians
	};

// Attributes
	int		GetTriangleCount() const;
	const GLYPH_TRIANGLE&	GetTriangle(int iTri) const;
	int		GetGlyphCount() const;
	int		GetGlyphTriangleCount(int iGlyph) const;
	float	GetTravel() const;
	void	SetRunBidiLevel(UINT nBidiLevel);

// Operations
	void	OnStartTrans(CKD2DSizeF szRT);
	void	OnDraw();
	HRESULT	TessellateGlyph(CKD2DPointF ptBaselineOrigin, const CKD2DRectF& rGlyph, const ID2D1PathGeometry *pPathGeom);
	void	GetNextGlyph(int& iStartTri, int& iEndTri);

// Overrides
	IFACEMETHOD(QueryInterface)(REFIID riid, void** ppv);
	IFACEMETHOD_(ULONG, AddRef)() { return 1; }
	IFACEMETHOD_(ULONG, Release)() { return 1; }
	IFACEMETHOD_(void, AddTriangles)(const D2D1_TRIANGLE* triangles, UINT32 trianglesCount);
	IFACEMETHOD(Close)() { return S_OK; }

protected:
// Data members
	CArrayEx<GLYPH_TRIANGLE, GLYPH_TRIANGLE&> m_aTriangle;	// array of glyph triangles
	CKD2DRectF	m_rGlyph;			// glyph bounds in world coordinates
	CKD2DPointF	m_ptGlyphCenterWorld;	// center of glyph in world coordinates
	CKD2DPointF	m_ptGlyphCenterLocal;	// center of glyph in local coordinates
	CKD2DSizeF	m_szRT;				// render target size
	CIntArrayEx	m_aGlyphTriCount;	// array of per-glyph triangle counts
	int		m_iGlyphFirstTri;	// index of glyph's first triangle within array
	int		m_iCurGlyph;		// index of current glyph for iterator
	float	m_fTravel;	// smallest radius that positions all triangles offscreen
	UINT	m_nRunBidiLevel;	// odd level indicates right-to-left language

// Helpers
	static float CalcTravel(const CKD2DSizeF& szRT, const DPoint& ptWorld, const DPoint& dir, float triRadius);
};

inline int CTriangleSink::GetTriangleCount() const
{
	return m_aTriangle.GetSize();
}

inline const CTriangleSink::GLYPH_TRIANGLE& CTriangleSink::GetTriangle(int iTri) const
{
	return m_aTriangle[iTri];
}

inline int CTriangleSink::GetGlyphCount() const
{
	return m_aGlyphTriCount.GetSize();
}

inline int CTriangleSink::GetGlyphTriangleCount(int iGlyph) const
{
	return m_aGlyphTriCount[iGlyph];
}

inline float CTriangleSink::GetTravel() const
{
	return m_fTravel;
}

inline void CTriangleSink::SetRunBidiLevel(UINT nBidiLevel)
{
	m_nRunBidiLevel = nBidiLevel;
}
