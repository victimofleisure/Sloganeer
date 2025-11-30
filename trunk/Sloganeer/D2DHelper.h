// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
		chris korda

		rev		date	comments
		00		21feb25	initial version
		01		27oct25	add size and inflate methods
		02		01nov25	add more constructors
		03		11nov25	add glyph iterator
		04		18nov25	add RGBA color init
		05		23nov25	add font metrics member to glyph iterator
		06		24nov25	add get origin method to glyph iterator
		07		27nov25	add tight vertical bounds flag to glyph iterator
		08		29nov25	add dips to pixels conversion methods
		09		30nov25	add ascent and descent attributes to glyph iterator

*/

#pragma once

#include "afxrendertarget.h"

inline D2D1_SIZE_F GetDpi(ID2D1RenderTarget* pRT)
{
	D2D1_SIZE_F	szDpi = {96.0f, 96.0f};
	if (pRT)
		pRT->GetDpi(&szDpi.width, &szDpi.height);
	return szDpi;
}

inline D2D1_SIZE_U DipsToPixels(ID2D1RenderTarget* pRT, D2D1_SIZE_F szDIPs, D2D1_SIZE_F szDpi)
{
	UINT	nX = static_cast<UINT>(ceilf(szDIPs.width * szDpi.width / 96.0f));
	UINT	nY = static_cast<UINT>(ceilf(szDIPs.height * szDpi.height / 96.0f));
	return D2D1::SizeU(nX, nY);
}

inline D2D1_SIZE_F PixelsToDips(ID2D1RenderTarget* pRT, D2D1_SIZE_U szPixels, D2D1_SIZE_F szDpi)
{
	float	fX = szPixels.width * 96.0f / szDpi.width;
	float	fY = szPixels.height * 96.0f / szDpi.height;
	return D2D1::SizeF(fX, fY);
}

inline D2D1_SIZE_U DipsToPixels(ID2D1RenderTarget* pRT, D2D1_SIZE_F szDIPs)
{
	return DipsToPixels(pRT, szDIPs, GetDpi(pRT));
}

inline D2D1_SIZE_F PixelsToDips(ID2D1RenderTarget* pRT, D2D1_SIZE_U szPixels)
{
	return PixelsToDips(pRT, szPixels, GetDpi(pRT));
}

class CKD2DRectF : public CD2DRectF {
public:
// Construction
	CKD2DRectF(const CRect& rect);
	CKD2DRectF(const D2D1_RECT_F& rect);
	CKD2DRectF(FLOAT fLeft = 0, FLOAT fTop = 0, FLOAT fRight = 0, FLOAT fBottom = 0);
	CKD2DRectF(const D2D1_POINT_2F& point, const D2D1_SIZE_F& size);
	CKD2DRectF(const D2D1_POINT_2F& ptTopLeft, const D2D1_POINT_2F& ptBottomRight);

// Attributes
	FLOAT	Width() const;
	FLOAT	Height() const;
	CD2DSizeF	Size() const;
	CD2DPointF	TopLeft() const;
	CD2DPointF	BottomRight() const;
	CD2DPointF	CenterPoint() const;
	bool	IsNormal() const;

// Operations
	void	OffsetRect(FLOAT dx, FLOAT dy);
	void	InflateRect(FLOAT dx, FLOAT dy);
	void	SetRectEmpty();
	bool	PtInRect(const D2D_POINT_2F& pt) const;
	void	Normalize();
	void	Union(const CKD2DRectF& r);
};

inline CKD2DRectF::CKD2DRectF(const CRect& rect) : CD2DRectF(rect)
{
}

inline CKD2DRectF::CKD2DRectF(const D2D1_RECT_F& rect) : CD2DRectF(rect)
{
}

inline CKD2DRectF::CKD2DRectF(FLOAT fLeft, FLOAT fTop, FLOAT fRight, FLOAT fBottom) 
{
	left = fLeft;
	top = fTop;
	right = fRight;
	bottom = fBottom;
}

inline CKD2DRectF::CKD2DRectF(const D2D1_POINT_2F& point, const D2D1_SIZE_F& size)
{
	left = point.x;
	top = point.y;
	right = point.x + size.width;
	bottom = point.y + size.height;
}

inline CKD2DRectF::CKD2DRectF(const D2D1_POINT_2F& ptTopLeft, const D2D1_POINT_2F& ptBottomRight)
{
	left = ptTopLeft.x;
	top = ptTopLeft.y;
	right = ptBottomRight.x;
	bottom = ptBottomRight.y;
}

inline FLOAT CKD2DRectF::Width() const
{ 
	return right - left;
}

inline FLOAT CKD2DRectF::Height() const
{ 
	return bottom - top;
}

inline CD2DSizeF CKD2DRectF::Size() const
{
	return CD2DSizeF(Width(), Height());
}

inline CD2DPointF CKD2DRectF::TopLeft() const
{
	return CD2DPointF(left, top);
}

inline CD2DPointF CKD2DRectF::BottomRight() const
{
	return CD2DPointF(bottom, right);
}

inline CD2DPointF CKD2DRectF::CenterPoint() const
{
	return CD2DPointF(left + Width() / 2, top + Height() / 2);
}

inline bool CKD2DRectF::IsNormal() const
{
	return left <= right && top <= bottom;
}

inline void CKD2DRectF::OffsetRect(FLOAT dx, FLOAT dy)
{ 
	left += dx;
	top += dy;
	right += dx;
	bottom += dy;
}

inline void CKD2DRectF::InflateRect(FLOAT dx, FLOAT dy)
{
	left -= dx;
	top -= dy;
	right += dx;
	bottom += dy;
}

inline void CKD2DRectF::SetRectEmpty() 
{ 
	left = 0;
	top = 0;
	right = 0;
	bottom = 0;
}

inline bool CKD2DRectF::PtInRect(const D2D_POINT_2F& pt) const
{ 
	return pt.x >= left && pt.y >= top && pt.x < right && pt.y < bottom;
}

inline void CKD2DRectF::Normalize()
{
	if (left > right)
		Swap(left, right);
	if (top > bottom)
		Swap(top, bottom);
}

inline void CKD2DRectF::Union(const CKD2DRectF& r)
{
	left = min(left, r.left);
	top = min(top, r.top);
	right = max(right, r.right);
	bottom = max(bottom, r.bottom);
}

class CGlyphIter {
public:
	CGlyphIter(CD2DPointF ptBaselineOrigin, DWRITE_GLYPH_RUN const* pGlyphRun, bool bTightVertBounds = false);
	//
	// Note that rGlyph is the painted area of the glyph, also known as its ink
	// box or black box, already in world coordinates. Do NOT use rGlyph as the
	// glyph origin for drawing; use GetOrigin() instead, and be sure to obtain 
	// the origin *before* calling GetNext(), which updates it.
	//
	bool	GetNext(UINT& iGlyph, CKD2DRectF& rGlyph);
	CD2DPointF	GetOrigin() const;
	float	GetAscent() const;
	float	GetDescent() const;
	void	Reset();

protected:
	CD2DPointF	m_ptOrigin;		// current origin
	DWRITE_GLYPH_RUN const* m_pGlyphRun;	// pointer to run
	float	m_fEmScale;		// scaling factor from font's design units to DIPs
	float	m_fAscent;		// scaled ascent
	float	m_fDescent;		// scaled descent
	UINT	m_iGlyph;		// index of current glyph
	float	m_fOriginX;		// original origin X for reset
	bool	m_bTightVertBounds;	// if true, compute tight vertical bounds, else use ascent
};

inline CD2DPointF CGlyphIter::GetOrigin() const
{
	return m_ptOrigin;
}

inline void CGlyphIter::Reset()
{
	m_iGlyph = 0;
	m_ptOrigin.x = m_fOriginX;
}

inline float CGlyphIter::GetAscent() const
{
	return m_fAscent;
}

inline float CGlyphIter::GetDescent() const
{
	return m_fDescent;
}

void	AddEllipse(ID2D1GeometrySink *pSink, D2D1_POINT_2F ptOrigin, D2D1_SIZE_F szRadius);
void	AddPieWedge(ID2D1GeometrySink *pSink, D2D1_POINT_2F ptOrigin, D2D1_SIZE_F szRadius, float fStartAngle, float fWedgeFrac);

inline D2D1::ColorF RGBAColorF(UINT32 rgba)
{
	// adapted from D2D1::ColorF constructor
	static const UINT32 shiftR = 24;
	static const UINT32 shiftG = 16;
	static const UINT32 shiftB = 8;
	static const UINT32 shiftA = 0;
	static const UINT32 maskR = 0xff << shiftR;
	static const UINT32 maskG = 0xff << shiftG;
	static const UINT32 maskB = 0xff << shiftB;
	static const UINT32 maskA = 0xff << shiftA;
	return D2D1::ColorF(
		static_cast<FLOAT>((rgba & maskR) >> shiftR) / 255.f,
		static_cast<FLOAT>((rgba & maskG) >> shiftG) / 255.f,
		static_cast<FLOAT>((rgba & maskB) >> shiftB) / 255.f,
		static_cast<FLOAT>((rgba & maskA) >> shiftA) / 255.f);
}
