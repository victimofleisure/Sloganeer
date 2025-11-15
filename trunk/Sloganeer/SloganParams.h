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

*/

#pragma once

class CSloganParams {
public:
// Construction
	CSloganParams();

// Public data
	CStringArrayEx	m_aSlogan;	// array of slogans to display
	bool	m_bStartFullScreen;	// true if starting in full screen mode
	bool	m_bSeqSlogans;		// true if showing slogans sequentially
	bool	m_bNoWordWrap;		// true if automatic word wrapping is disabled
	int		m_nHoldDuration;	// hold duration in milliseconds
	int		m_nPauseDuration;	// pause duration in milliseconds
	float	m_fInTransDuration;	// incoming transition duration in seconds
	float	m_fOutTransDuration;	// outgoing transition duration in seconds
	CString	m_sFontName;		// font name
	float	m_fFontSize;		// font size, in points
	int		m_nFontWeight;		// font weight, from 1 to 999
	D2D1::ColorF	m_clrBkgnd;	// background color
	D2D1::ColorF	m_clrDraw;	// drawing color
	UINT	m_nRandSeed;		// starting point for random number generation
	float	m_fEasing;			// fraction of motion to ease, from 0 to 1
	CString	m_sRecFolderPath;	// recording destination folder path
	CSize	m_szRecFrameSize;	// recording frame size in pixels
	float	m_fRecFrameRate;	// recording frame rate
	float	m_fRecDuration;		// recording duration in seconds

// Attributes
	void	SetSlogans(const LPCTSTR *aSlogan, int nSlogans);
	bool	IsRecording() const;

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
