// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
		chris korda

		rev		date		comments
		00		21feb25	initial version
		01		27oct25	add size and inflate methods
		02		01nov25	add more constructors

*/

#pragma once

#include "afxrendertarget.h"

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

// Operations
	void	OffsetRect(FLOAT dx, FLOAT dy);
	void	InflateRect(FLOAT dx, FLOAT dy);
	void	SetRectEmpty();
	bool	PtInRect(const D2D_POINT_2F& pt) const;
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
