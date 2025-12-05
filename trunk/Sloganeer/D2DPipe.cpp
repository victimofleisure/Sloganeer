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

#include "stdafx.h"
#include "D2DPipe.h"
#include "D2DHelper.h"
#include "Event.h"

#define CHECK(x) { HRESULT hr = x; if (FAILED(hr)) { OnError(hr, __FILE__, __LINE__, __DATE__); return false; }}

CD2DPipe::CD2DPipe()
{
	m_hPipe = INVALID_HANDLE_VALUE;
}

CD2DPipe::~CD2DPipe()
{
	ClosePipe();
}

bool CD2DPipe::OnConnectTimeout()
{
	return false;
}

bool CD2DPipe::CreatePipe(LPCTSTR pszPipeName, DWORD dwTimeout)
{
	ASSERT(IsCreated());	// must create base class first
	if (!IsCreated())	// if offscreen instance doesn't exist
		return false;	// usage error
	ASSERT(!IsPipeCreated());	// already existing pipe is a usage error
	if (!CheckPitch())	// arrange frame packing if needed
		return false;
	DWORD	dwOpenMode = PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED;
	DWORD	dwPipeMode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT;
	m_hPipe = CreateNamedPipe(
		pszPipeName, dwOpenMode, dwPipeMode, 1,	// max instances
		0, 0, 0, NULL);	// output and input buffers, no client timeout
	if (!IsPipeCreated())	// create pipe
		return false;
	return ConnectPipe(dwTimeout);	// wait for connection
}

bool CD2DPipe::ConnectPipe(DWORD dwTimeout)
{
	ASSERT(IsPipeCreated());
	WEvent	evt;
	evt.Create(NULL, TRUE, FALSE, NULL);	// manual reset
	if (evt == NULL)	// if can't create event
		return false;
	OVERLAPPED ov = {};
	ov.hEvent = evt;
	if (ConnectNamedPipe(m_hPipe, &ov))	// try to connect pipe
		return true;	// immediate success
	DWORD	dwError = GetLastError();
	if (dwError == ERROR_PIPE_CONNECTED)	// if connected despite alleged error
		return true;	// success
	if (dwError != ERROR_IO_PENDING)	// if any error other than I/O pending
		return false;	// fail
	// loop until connected, or fatal error, or timeout handler cancels
	while (1) {
		// wait for connection
		DWORD	dwRet = WaitForSingleObject(ov.hEvent, dwTimeout);
		if (dwRet == WAIT_OBJECT_0)	// if overlapped I/O event signaled
			break;	// connection is established
		// if wait returned an error, or timeout handler canceled
		if (dwRet != WAIT_TIMEOUT || !OnConnectTimeout())
			return false;	// fail
	}
	return true;	// success
}

void CD2DPipe::ClosePipe()
{
	if (IsPipeCreated()) {	// if pipe exists
		CloseHandle(m_hPipe);
		m_hPipe = INVALID_HANDLE_VALUE;
	}
}

bool CD2DPipe::CheckPitch()
{
	// don't need to copy resource just to check its layout
	D3D11_MAPPED_SUBRESOURCE map = {};
	CHECK(m_pD3DDeviceContext->Map(m_pStaging, 0, D3D11_MAP_READ, 0, &map));
	CAutoUnmapResource	unmapStaging(m_pD3DDeviceContext, m_pStaging);	// dtor unmaps staging
	UINT	nPackedPitch = m_szImage.width * sizeof(DWORD);	// pitch without any padding
	if (map.RowPitch != nPackedPitch) {	// if actual pitch isn't same as packed pitch
		m_aPackedFrame.SetSize(nPackedPitch * m_szImage.height);	// allocate packing buffer
	} else {	// no packing required
		m_aPackedFrame.RemoveAll();	// no buffer needed either
	}
	return true;
}

bool CD2DPipe::WritePipe()
{
	m_pD3DDeviceContext->CopyResource(m_pStaging, m_pRenderTex);
	D3D11_MAPPED_SUBRESOURCE map = {};
	CHECK(m_pD3DDeviceContext->Map(m_pStaging, 0, D3D11_MAP_READ, 0, &map));
	CAutoUnmapResource	unmapStaging(m_pD3DDeviceContext, m_pStaging);	// dtor unmaps staging
	const BYTE*	pFrameData = static_cast<const BYTE*>(map.pData);
	UINT	nWidth = m_szImage.width;
	UINT	nHeight = m_szImage.height;
	UINT	nFrameSize;
	if (!m_aPackedFrame.IsEmpty()) {	// if frame needs to be packed
		UINT	nPackedPitch = nWidth * sizeof(DWORD);	// pitch without any padding
		const BYTE*	pSrc = pFrameData;	// source is mapped frame
		BYTE*	pDst = m_aPackedFrame.GetData();	// destination is packed frame
		for (UINT iRow = 0; iRow < nHeight; iRow++) {	// for each frame row
			memcpy(pDst, pSrc, nPackedPitch);	// copy row bytes
			pSrc += map.RowPitch;	// bump source pointer
			pDst += nPackedPitch;	// bump destination pointer
		}
		// write from packed frame
		pFrameData = m_aPackedFrame.GetData();
		nFrameSize = nPackedPitch * nHeight;
	} else {	// not packing
		// write directly from mapped frame
		nFrameSize = map.RowPitch * nHeight;
	}
	DWORD	nBytesWritten;
	BOOL	bResult = WriteFile(m_hPipe, pFrameData, nFrameSize, &nBytesWritten, NULL);
	return bResult && nBytesWritten == nFrameSize;
}
