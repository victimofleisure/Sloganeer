// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      28oct25	initial version

*/

#pragma once

#include "D2DDevCtx.h"
#include "ILockRingBuf.h"
#include "Event.h"

class CD2DCapture {
public:
// Construction
	CD2DCapture();
	virtual ~CD2DCapture();

// Attributes
	bool	IsCreated() const;
	void	SetFormat(GUID guidContainerFormat, LPCTSTR pszFileExtension);

// Operations
	bool	Create(ID3D11Device* pDevice, IDXGISwapChain1* pSwapChain, 
		LPCTSTR pszOutFolderPath = NULL, int nWriters = 4, int nRingBufSize = 32);
	bool	Destroy();
	bool	CaptureFrame();

// Overrideables
	virtual void	OnError(HRESULT hr, LPCSTR pszSrcFileName, int nLineNum, LPCSTR pszSrcFileDate);

protected:
// Types
	struct FRAME {
	public:
		BYTE	*pData;		// pointer to frame data
		int		nWidth;		// width in pixels
		int		nHeight;	// height in pixels
		int		nStride;	// row pitch in bytes
		int		iFrame;		// frame index
	};
	class CWriter {
	public:
	// Construction
		CWriter();

	// Attributes
		bool	IsCreated() const;

	// Operations
		bool	Create(CD2DCapture *pCapture);
		void	Destroy();
		bool	PushFrame(const FRAME& frame);

	protected:
	// Member data
		CILockRingBuf<FRAME>	m_rbFrame;	// frame ring buffer
		CComPtr<IWICImagingFactory>	m_pWICFactory;	// WIC imaging factory
		CAutoPtr<CWinThread>	m_pWorker;	// pointer to worker thread
		WEvent	m_evtWrite;		// write signal
		CD2DCapture*	m_pCapture;	// pointer to parent capture instance

	// Helpers
		void	OnError(HRESULT hr, LPCSTR pszSrcFileName, int nLineNum, LPCSTR pszSrcFileDate);
		bool	WriteFrame(const FRAME& frame);
		bool	ThreadFunc();
		static UINT	ThreadFunc(LPVOID pParam);
	};
	typedef CArrayEx<CWriter, CWriter&> CWriterArray;

// Constants
	enum {
		STAGING_BUFFERS = 2,	// number of staging buffers in chain
	};

// Member data
	CComPtr<ID3D11DeviceContext>	m_pD3DDeviceContext;	// Direct3D device context
	CComPtr<ID3D11Texture2D>	m_aStaging[STAGING_BUFFERS];	// array of staging buffers
	CWriterArray	m_aWriter;	// array of writers
	ID3D11Device*	m_pD3DDevice;	// pointer to Direct3D device
	IDXGISwapChain1*	m_pSwapChain;	// pointer to DXGI swap chain
	CString	m_sOutFolderPath;	// path to output folder
	CSize	m_szFrame;			// frame size in pixels
	int		m_nRingBufSize;		// size of writer ring buffer in frames
	int		m_iWriter;			// index of current writer
	int		m_iStagingBuf;		// index of current staging buffer
	int		m_nStagedFrames;	// number of frames staged
	int		m_nCapturedFrames;	// number of frames captured
	bool	m_bExitFlag;		// true if destroying writers
	GUID	m_guidContainerFormat;	// identifier of WIC container format
	LPCTSTR	m_pszFileExtension;	// file extension appropriate for format

// Helpers
	void	Init();
	void	PushFrame(const FRAME& frame);
};

inline bool CD2DCapture::CWriter::IsCreated() const
{
	return m_pWorker != NULL;
}

inline bool CD2DCapture::IsCreated() const
{
	int	nWriters = m_aWriter.GetSize();
	return (nWriters && m_aWriter[nWriters - 1].IsCreated());
}

inline void CD2DCapture::SetFormat(GUID guidContainerFormat, LPCTSTR pszFileExtension)
{
	ASSERT(pszFileExtension != NULL);
	m_guidContainerFormat = guidContainerFormat;
	m_pszFileExtension = pszFileExtension;
}
