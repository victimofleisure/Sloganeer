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

#pragma once

class CTextRenderer : public IDWriteTextRenderer {
public:
// Construction
	CTextRenderer();
	virtual ~CTextRenderer();

// Overrides
	IFACEMETHOD_(ULONG, AddRef)();
	IFACEMETHOD_(ULONG, Release)();
	IFACEMETHOD(QueryInterface)(REFIID riid, void** ppvObject);
	IFACEMETHOD(DrawInlineObject)(void* pClientDrawingContext, FLOAT fOriginX, FLOAT fOriginY, 
		IDWriteInlineObject* pInlineObject, BOOL bIsSideways, BOOL bIsRightToLeft, IUnknown* pClientDrawingEffect);
	IFACEMETHOD(DrawStrikethrough)(void* pClientDrawingContext, FLOAT fBaselineOriginX, 
		FLOAT fBaselineOriginY, DWRITE_STRIKETHROUGH const* pStrikethrough, IUnknown* pClientDrawingEffect);
	IFACEMETHOD(DrawUnderline)(void* pClientDrawingContext, FLOAT fBaselineOriginX, 
		FLOAT fBaselineOriginY, DWRITE_UNDERLINE const* pUnderline, IUnknown* pClientDrawingEffect);
	IFACEMETHOD(IsPixelSnappingDisabled)(void* pClientDrawingContext, BOOL* pbIsDisabled);
	IFACEMETHOD(GetCurrentTransform)(void* pClientDrawingContext, DWRITE_MATRIX* pTransform);
	IFACEMETHOD(GetPixelsPerDip)(void* pClientDrawingContext, FLOAT* pfPixelsPerDip);
    IFACEMETHOD(DrawGlyphRun)(void* pClientDrawingContext, FLOAT fBaselineOriginX, 
		FLOAT fBaselineOriginY, DWRITE_MEASURING_MODE measuringMode, DWRITE_GLYPH_RUN const* pGlyphRun, 
		DWRITE_GLYPH_RUN_DESCRIPTION const* pGlyphRunDescription, IUnknown* pClientDrawingEffect);

protected:
// Member data
	ULONG	m_nRefCount;	// reference count
};
