// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      30oct25	initial version

		skeletal base class for DirectWrite text renderer

*/

#include "stdafx.h"
#include "TextRenderer.h"

CTextRenderer::CTextRenderer()
{
	m_nRefCount = 1;
}

CTextRenderer::~CTextRenderer()
{
}

ULONG CTextRenderer::AddRef()
{
	return ++m_nRefCount;	// add a reference
}

ULONG CTextRenderer::Release()
{
	ULONG nNewCount = --m_nRefCount;	// remove a reference
	if (m_nRefCount == 0)	// if no more references
		delete this;	// self-delete
	return nNewCount;
}

HRESULT CTextRenderer::QueryInterface(REFIID riid, void** ppvObject)
{
	if (riid == __uuidof(IDWriteTextRenderer) || riid == __uuidof(IUnknown)) {
		*ppvObject = this; 
		AddRef(); 
		return S_OK;
	}
	*ppvObject = NULL;
	return E_NOINTERFACE;
}

HRESULT CTextRenderer::DrawInlineObject(void* pClientDrawingContext, FLOAT fOriginX, FLOAT fOriginY, 
	IDWriteInlineObject* pInlineObject, BOOL bIsSideways, BOOL bIsRightToLeft, IUnknown* pClientDrawingEffect)
{
	return S_OK;
}

HRESULT CTextRenderer::DrawStrikethrough(void* pClientDrawingContext, FLOAT fBaselineOriginX, 
	FLOAT fBaselineOriginY, DWRITE_STRIKETHROUGH const* pStrikethrough, IUnknown* pClientDrawingEffect)
{
	return S_OK;
}

HRESULT CTextRenderer::DrawUnderline(void* pClientDrawingContext, FLOAT fBaselineOriginX, 
	FLOAT fBaselineOriginY, DWRITE_UNDERLINE const* pUnderline, IUnknown* pClientDrawingEffect)
{
	return S_OK;
}

HRESULT CTextRenderer::IsPixelSnappingDisabled(void* pClientDrawingContext, BOOL* pbIsDisabled)
{
	*pbIsDisabled = FALSE;
	return S_OK;
}

HRESULT CTextRenderer::GetCurrentTransform(void* pClientDrawingContext, DWRITE_MATRIX* pTransform)
{
	DWRITE_MATRIX	m = {1, 0, 0, 1, 0, 0};
	*pTransform = m;
	return S_OK;
}

HRESULT CTextRenderer::GetPixelsPerDip(void* pClientDrawingContext, FLOAT* pfPixelsPerDip)
{
	*pfPixelsPerDip = 1.0f;
	return S_OK;
}

HRESULT CTextRenderer::DrawGlyphRun(void* pClientDrawingContext, FLOAT fBaselineOriginX, 
	FLOAT fBaselineOriginY, DWRITE_MEASURING_MODE measuringMode, DWRITE_GLYPH_RUN const* pGlyphRun, 
	DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, IUnknown* pClientDrawingEffect)
{
	return S_OK;
}
