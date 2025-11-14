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

#include "stdafx.h"
#include "resource.h"
#include "D2DCapture.h"
#include "PathStr.h"

#define CHECK(x) { HRESULT hr = x; if (FAILED(hr)) { OnError(hr, __FILE__, __LINE__, __DATE__); return false; }}

CD2DCapture::CD2DCapture()
{
	m_pD3DDevice = NULL;
	m_pSwapChain = NULL;
	Init();
}

CD2DCapture::~CD2DCapture()
{
	Destroy();
}

void CD2DCapture::Init()
{
	m_szFrame = CSize(0, 0);
	m_nRingBufSize = 0;
	m_iWriter = 0;
	m_iStagingBuf = 0;
	m_nStagedFrames = 0;
	m_nCapturedFrames = 0;
	m_bExitFlag = false;
	m_guidContainerFormat = GUID_ContainerFormatTiff;
	m_pszFileExtension = _T(".tif");
}

void CD2DCapture::OnError(HRESULT hr, LPCSTR pszSrcFileName, int nLineNum, LPCSTR pszSrcFileDate)
{
	UNREFERENCED_PARAMETER(hr);
	UNREFERENCED_PARAMETER(pszSrcFileName);
	UNREFERENCED_PARAMETER(nLineNum);
	UNREFERENCED_PARAMETER(pszSrcFileDate);
}

bool CD2DCapture::Create(ID3D11Device* pDevice, IDXGISwapChain1* pSwapChain, LPCTSTR pszOutFolderPath, int nWriters, int nRingBufSize)
{
	ASSERT(pDevice != NULL);
	ASSERT(pSwapChain != NULL);
	ASSERT(nWriters > 0);
	ASSERT(nRingBufSize > 0);
	Init();
	m_pD3DDevice = pDevice;
	m_pSwapChain = pSwapChain;
	m_nRingBufSize = nRingBufSize;
	m_sOutFolderPath = pszOutFolderPath;
	CComPtr<IDXGIDevice1> pDXGIDevice;
	CHECK(m_pD3DDevice->QueryInterface(IID_PPV_ARGS(&pDXGIDevice)));
	//
	// DXGI normally hoards up to three frames in order to maximize
	// parallelism, but that blocks the copying of resources to CPU
	// memory; by limiting the DXGI queue to a single frame, we gain
	// fast back buffer copying, possibly at the expense of overall
	// throughput. See CopyResource, performance considerations, and
	// Copying and Accessing Resource Data in MSDN.
	//
	CHECK(pDXGIDevice->SetMaximumFrameLatency(1));	// limit DXGI queue to a single frame
	m_pD3DDevice->GetImmediateContext(&m_pD3DDeviceContext);
	CComPtr<ID3D11Texture2D>	pBackbuf;
	CHECK(m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackbuf)));
	D3D11_TEXTURE2D_DESC desc;
	pBackbuf->GetDesc(&desc);
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_STAGING;	// texture usage is staging buffer
	desc.BindFlags = 0;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;	// CPU will read this buffer
	desc.MiscFlags = 0;
	for (int iBuf = 0; iBuf < STAGING_BUFFERS; iBuf++) {	// for each staging buffer
		CHECK(m_pD3DDevice->CreateTexture2D(&desc, NULL, &m_aStaging[iBuf]));
	}
	m_szFrame = CSize(desc.Width, desc.Height);	// copy frame size
	m_aWriter.SetSize(nWriters);	// allocate requested number of writers
	for (int iWriter = 0; iWriter < m_aWriter.GetSize(); iWriter++) {	// for each writer
		if (!m_aWriter[iWriter].Create(this))	// create writer
			return false;
	}
	return true;
}

bool CD2DCapture::Destroy()
{
	m_bExitFlag = true;
	for (int iWriter = 0; iWriter < m_aWriter.GetSize(); iWriter++) {	// for each writer
		m_aWriter[iWriter].Destroy();	// destroy writer
	}
	m_aWriter.RemoveAll();
	m_pD3DDeviceContext.Release();
	for (int iBuf = 0; iBuf < STAGING_BUFFERS; iBuf++) {
		m_aStaging[iBuf].Release();
	}
	m_pD3DDevice = NULL;
	m_pSwapChain = NULL;
	return true;
}

bool CD2DCapture::CaptureFrame()
{
	ASSERT(IsCreated());	// else call is improper
	//
	// Copy frame from back buffer to current staging buffer; CopyResource is
	// asynchronous and the copy is not necessarily completed by the time the
	// method returns, hence the need for a chain of staging buffers.
	//
	CComPtr<ID3D11Texture2D>	pBackbuf;
	CHECK(m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackbuf)));
	m_pD3DDeviceContext->CopyResource(m_aStaging[m_iStagingBuf], pBackbuf);
	// compute index of least recently used staging buffer, which (provided
	// buffer chain is full) contains a frame ready to be mapped and copied
	int	iLRUStaging = m_iStagingBuf - 1;
	if (iLRUStaging < 0)	// if negative buffer index
		iLRUStaging = STAGING_BUFFERS - 1;	// wrap to last buffer
	m_iStagingBuf++;	// bump staging buffer index
	if (m_iStagingBuf >= STAGING_BUFFERS)	// if buffer index beyond last
		m_iStagingBuf = 0;	// wrap to first buffer
	m_nStagedFrames++;	// bump staged frame count
	if (m_nStagedFrames < STAGING_BUFFERS)	// if not enough frames staged
		return true;	// wait for staging buffer chain to fill up
	D3D11_TEXTURE2D_DESC	desc;
	pBackbuf->GetDesc(&desc);
	CSize	szFrame(desc.Width, desc.Height);
	if (szFrame != m_szFrame) {	// if frame size changed, fatal error
		AfxMessageBox(IDS_ERR_CAPTURE_FRAME_SIZE_CHANGE);
		return false;	// frame resize during capture is unsupported
	}
	FRAME	frame;
	//
	// Code between Map and Unmap copies the frame data from the staging
	// buffer to the frame buffer in CPU memory; unlike CopyResouce, this
	// is synchronous and can potentially stall the GPU pipeline, though
	// we hopefully avoid a stall by using a queue of staging buffers,
	// and by limiting DXGI to a single frame via SetMaximumFrameLatency.
	//
	D3D11_MAPPED_SUBRESOURCE map;
	CHECK(m_pD3DDeviceContext->Map(m_aStaging[iLRUStaging], // map staging buffer
		0, D3D11_MAP_READ, 0, &map));   // blocks until copy completes
	frame.pData = new BYTE[map.RowPitch * szFrame.cy];	// allocate frame data
	memcpy(frame.pData, map.pData, map.RowPitch * szFrame.cy);	// copy frame data
	m_pD3DDeviceContext->Unmap(m_aStaging[iLRUStaging], 0);	// unmap staging buffer
	// frame is copied; staging buffer is unmapped and available for reuse
	frame.nWidth = szFrame.cx;
	frame.nHeight = szFrame.cy;
	frame.nStride = map.RowPitch;
	frame.iFrame = m_nCapturedFrames;
	m_nCapturedFrames++;	// bump captured frame index
	PushFrame(frame);
	return true;
}

void CD2DCapture::PushFrame(const FRAME& frame)
{
	while (1) {
		const int	nWriters = m_aWriter.GetSize();
		for (int iWriter = 0; iWriter < nWriters; iWriter++) {	// for each writer
			int	iTryWriter = m_iWriter;	// index of writer to try
			m_iWriter++;	// bump writer index
			if (m_iWriter >= m_aWriter.GetSize())	// if writer index beyond last
				m_iWriter = 0;	// wrap around to first writer
			// push frame into writer thread's ring buffer; if push fails,
			// ring buffer is full, so keep looping and try other writers
			if (m_aWriter[iTryWriter].PushFrame(frame))	// if push frame succeeds
				return;	// frame found a taker, we're done
		}
		// all writers are full
		Sleep(1);	// take a break before trying them again
	}
}

CD2DCapture::CWriter::CWriter()
{
	m_pCapture = NULL;
}

void CD2DCapture::CWriter::OnError(HRESULT hr, LPCSTR pszSrcFileName, int nLineNum, LPCSTR pszSrcFileDate)
{
	m_pCapture->OnError(hr, pszSrcFileName, nLineNum, pszSrcFileDate);
}

bool CD2DCapture::CWriter::Create(CD2DCapture *pCapture)
{
	ASSERT(pCapture != NULL);
	m_pCapture = pCapture;	// copy capture instance to member
	m_rbFrame.Create(pCapture->m_nRingBufSize);	// create frame ring buffer
	if (!m_evtWrite.Create(NULL, false, false, NULL))	// create write event; auto reset
		return false;
	// create thread suspended so we can safely clear its auto delete flag
	CWinThread*	pThread = AfxBeginThread(ThreadFunc, this, 0, 0, CREATE_SUSPENDED);
	if (pThread == NULL)	// if can't create thread
		return false;
	m_pWorker.Attach(pThread);	// attach thread instance to member
	pThread->m_bAutoDelete = false;	// clear auto delete flag
	pThread->ResumeThread();	// launch thread
	return true;
}

void CD2DCapture::CWriter::Destroy()
{
	if (m_pWorker == NULL)	// if worker not launched
		return;	// nothing to do
	FRAME	frame = {0};
	m_rbFrame.Push(frame);	// push an empty frame sentinel, indicating end of frames
	m_evtWrite.Set();	// wake worker thread if it's asleep
	WaitForSingleObject(m_pWorker->m_hThread, INFINITE);	// wait for worker thread to exit
	m_pWorker.Free();	// free worker thread instance
}

bool CD2DCapture::CWriter::PushFrame(const FRAME& frame)
{
	if (!m_rbFrame.Push(frame))	// if frame ring buffer full
		return false;	// let caller handle it
	m_evtWrite.Set();	// wake worker thread if it's asleep
	return true;
}

bool CD2DCapture::CWriter::WriteFrame(const FRAME& frame)
{
	ASSERT(frame.pData != NULL);
	CString	sImageFileName;
	sImageFileName.Format(_T("%06d"), frame.iFrame);
	CPathStr	sImagePath(m_pCapture->m_sOutFolderPath);
	sImagePath.Append(_T("cap") + sImageFileName + m_pCapture->m_pszFileExtension);	// make image file path
	CComPtr<IWICBitmapEncoder> pEncoder;
	CHECK(m_pWICFactory->CreateEncoder(m_pCapture->m_guidContainerFormat, NULL, &pEncoder));	// create encoder
	CComPtr<IWICStream> pStream;
	CHECK(m_pWICFactory->CreateStream(&pStream));	// create WIC stream
	CHECK(pStream->InitializeFromFilename(sImagePath, GENERIC_WRITE)); // initialize stream
	CHECK(pEncoder->Initialize(pStream, WICBitmapEncoderNoCache));	// initialize encoder with stream
	CComPtr<IWICBitmapFrameEncode>	pFrame;
	CHECK(pEncoder->CreateNewFrame(&pFrame, NULL));	// create WIC frame
	CHECK(pFrame->Initialize(NULL));	// initialize frame encoder
	CHECK(pFrame->SetSize(frame.nWidth, frame.nHeight));	// set desired image dimensions
	WICPixelFormatGUID formatGUID = GUID_WICPixelFormat32bppBGRA;	// pixel format GUID
	CHECK(pFrame->SetPixelFormat(&formatGUID));	// set desired pixel format
	UINT cbStride = frame.nStride;	// size of bitmap scanline in bytes
	UINT cbBufferSize = cbStride * frame.nHeight;	// buffer size is pitch times height
	pFrame->WritePixels(frame.nHeight, cbStride, cbBufferSize, static_cast<BYTE*>(frame.pData));	// encode frame scanlines
	CHECK(pFrame->Commit());	// commit frame to image
	CHECK(pEncoder->Commit());	// commit all image changes and close stream
	return true;
}

bool CD2DCapture::CWriter::ThreadFunc()
{
	CHECK(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED));
	CHECK(CoCreateInstance(CLSID_WICImagingFactory, NULL,	// create WIC factory
		CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_pWICFactory)));
	while (1) {
		WaitForSingleObject(m_evtWrite, INFINITE);	// wait for write signal
		FRAME	frame;
		while (m_rbFrame.Pop(frame)) {	// while frames can be dequeued
			if (frame.pData == NULL)	// if end of frames sentinel
				goto lblEndOfFrames;	// exit loop, we're done
			WriteFrame(frame);	// write frame
			delete [] frame.pData;	// delete frame's data
			frame.pData = NULL;	// mark frame as deleted
		}
	}
lblEndOfFrames:
	m_pWICFactory.Release();
	CoUninitialize();
	return true;
}

UINT CD2DCapture::CWriter::ThreadFunc(LPVOID pParam)
{
	CWriter	*pThis = static_cast<CWriter*>(pParam);
	pThis->ThreadFunc();
	return 0;
}
