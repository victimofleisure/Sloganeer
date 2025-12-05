// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00		03dec25	initial version
		
*/

#pragma once

#include "D2DOffscreen.h"

class CD2DPipe : public CD2DOffscreen {
public:
// Construction
	CD2DPipe();
	virtual ~CD2DPipe();

// Attributes
	bool	IsPipeCreated() const;

// Operations
	bool	CreatePipe(LPCTSTR pszPipeName, DWORD dwTimeout = 5000);
	bool	WritePipe();
	void	ClosePipe();

// Overrideables
	virtual bool	OnConnectTimeout();

protected:
// Member data
	HANDLE	m_hPipe;	// pipe handle
	CByteArrayEx	m_aPackedFrame;	// frame buffer for packing

// Helpers
	bool	CheckPitch();
	bool	ConnectPipe(DWORD dwTimeout = 5000);
};

inline bool CD2DPipe::IsPipeCreated() const
{
	return m_hPipe != INVALID_HANDLE_VALUE;
}
