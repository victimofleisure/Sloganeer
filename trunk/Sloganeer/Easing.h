// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      05nov25	initial version

*/

#pragma once

// s is the normalized progress of the motion, from 0 to 1
// F is the fraction of the motion to ease, from 0 to 1

double EaseIn(double s, double F);
double EaseOut(double s, double F);

inline double EaseInOut(bool bIsOut, double s, double F)
{
	return bIsOut ? EaseOut(s, F) : EaseIn(s, F);
}
