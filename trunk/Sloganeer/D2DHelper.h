// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
		chris korda

		rev		date		comments
		00		21feb25	initial version
		01		27oct25	add size and inflate methods

*/

#pragma once

#include "afxrendertarget.h"

class CKD2DRectF : public CD2DRectF {
public:
	CKD2DRectF(const CRect& rect);
	CKD2DRectF(const D2D1_RECT_F& rect);
	CKD2DRectF(FLOAT fLeft = 0, FLOAT fTop = 0, FLOAT fRight = 0, FLOAT fBottom = 0);
	FLOAT	Width() const;
	FLOAT	Height() const;
	void	OffsetRect(float dx, float dy);
	void	InflateRect(float dx, float dy);
	void	SetRectEmpty();
	bool	PtInRect(const D2D_POINT_2F& pt) const;
	CD2DSizeF	Size() const;
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

inline FLOAT CKD2DRectF::Width() const
{ 
	return right - left;
}

inline FLOAT CKD2DRectF::Height() const
{ 
	return bottom - top;
}

inline void CKD2DRectF::OffsetRect(float dx, float dy)
{ 
	left += dx;
	top += dy;
	right += dx;
	bottom += dy;
}

inline void CKD2DRectF::InflateRect(float dx, float dy)
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

inline CD2DSizeF CKD2DRectF::Size() const
{
	return CD2DSizeF(Width(), Height());
}
