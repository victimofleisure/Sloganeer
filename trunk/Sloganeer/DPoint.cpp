// Copyleft 2005 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
		chris korda

		rev		date	comments
		00		28jan03	initial version
        01		22jun17	add length, distance and intersect methods
		02		03mar25	modernize style

		double-precision 2D coordinate

*/

#include "stdafx.h"
#include "DPoint.h"
#define _USE_MATH_DEFINES
#include "math.h"

const double DPoint::m_fEpsilon = 1e-10;

bool DPoint::Equal(double a, double b)
{
	return fabs(a - b) < m_fEpsilon;	// less than this and they're equal
}

DPoint DPoint::Intersect(DPoint p1, DPoint p2, DPoint& vIntersect) const
{
	DPoint	vL(p2 - p1);	// normalize line segment
	double	fThetaL = -M_PI / 2 - atan2(vL.y, vL.x);	// angle of line segment
	DPoint	vC(p1 - *this);	// normalize hypotenuse
	double	fThetaC = -M_PI / 2 - atan2(vC.y, vC.x);	// angle of hypotenuse
	double	fLenC = vC.Length();	// length of hypotenuse
	double	fLenAdj = cos(fThetaL - fThetaC) * fLenC;	// length of adjacent side
	// rotate adjacent side to be colinear with input line segment
	vIntersect = DPoint(sin(fThetaL), cos(fThetaL));	// intersection vector
	return vIntersect * fLenAdj + p1;	// return intersection point
}

void DPoint::Rotate(DPoint ptOrigin, double fRotation)
{
	double	s = sin(fRotation);
	double	c = cos(fRotation);
	*this -= ptOrigin;
	*this = DPoint(x * c - y * s, y * c + x * s) + ptOrigin;
}
