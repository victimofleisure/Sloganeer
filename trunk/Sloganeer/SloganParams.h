// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      30oct25	initial version
        01      12nov25	add easing
		02		14nov25	add recording
		03		15nov25	add color names
		04		18nov25	add CSV support
        05      25nov25	add color palette and cycling
		06		03dec25	add pipe name attribute

*/

#pragma once

#include "Slogan.h"

class CSloganParams : public CSlogan {
public:
// Construction
	CSloganParams();

// Types
	class CPalette : public CArrayEx<D2D1_COLOR_F, D2D1_COLOR_F&> {
	public:
	// Construction
		CPalette();

	// Operations
		void	Read(LPCTSTR pszPath);
		void	CycleColor(double fElapsedTime, D2D1::ColorF& color) const;

	// Public data
		double	m_fCycleFreq;	// color cycling frequency in Hertz
	};

// Public data
	CSloganArray	m_aSlogan;	// array of slogans to display
	bool	m_bStartFullScreen;	// true if starting in full screen mode
	bool	m_bSeqSlogans;		// true if showing slogans sequentially
	bool	m_bNoWordWrap;		// true if automatic word wrapping is disabled
	bool	m_bCustomSlogans;	// true if doing per-slogan customization
	UINT	m_nRandSeed;		// starting point for random number generation
	float	m_fEasing;			// fraction of motion to ease, from 0 to 1
	CString	m_sRecFolderPath;	// recording destination folder path
	CSize	m_szRecFrameSize;	// recording frame size in pixels
	float	m_fRecFrameRate;	// recording frame rate
	float	m_fRecDuration;		// recording duration in seconds
	CPalette	m_palBkgnd;		// background color palette
	CPalette	m_palDraw;		// drawing color palette

// Attributes
	void	SetSlogans(const LPCTSTR *aSlogan, int nSlogans);
	bool	IsRecording() const;
	static bool	IsPipeName(CString sPath);

// Operations
	void	ReadSlogans(LPCTSTR pszPath);
	static COLORREF	FindColor(LPCTSTR pszName);
	static W64INT BinarySearch(const LPCTSTR* aStr, W64INT nStrings, LPCTSTR pszTarget);

// Constants
	static const LPCTSTR m_aColorName[];
	static const COLORREF m_aColorVal[];
};

inline bool CSloganParams::IsRecording() const
{
	return !m_sRecFolderPath.IsEmpty();
}
