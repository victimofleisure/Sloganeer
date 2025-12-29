// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      24oct25	initial version
		01		29oct25	add pause and color parameters
		02		30oct25	add nowrap parameter
        03      12nov25	add easing
		04		14nov25	add recording
        05      18nov25	add slogan members
        06      25nov25	add color palette and cycling
        07      27nov25	add submarine transition
		08		02dec25	add text and transition type
		09		15dec25	add tumble transition
		10		20dec25	add iris transition
		11		21dec25	refactor hold and pause
		12		29dec25	add blur transition

*/

#ifdef PARAMDEF

PARAMDEF(help)			// display help
PARAMDEF(fullscr)		// start application in full screen mode
PARAMDEF(text)			// slogan text
PARAMDEF(fontsize)		// font size in DIPs
PARAMDEF(fontname)		// font name
PARAMDEF(fontwt)		// font weight, from 1 - 999
PARAMDEF(transdur)		// transition duration in seconds
PARAMDEF(holddur)		// hold duration in seconds
PARAMDEF(outdur)		// outgoing transition duration in seconds
PARAMDEF(pausedur)		// pause duration in seconds
PARAMDEF(seqtext)		// display slogans in sequential order instead of randomizing them
PARAMDEF(bgclr)			// background color
PARAMDEF(bgpal)			// background color palette file path
PARAMDEF(bgfrq)			// background color cycling frequency in Hertz
PARAMDEF(drawclr)		// drawing color
PARAMDEF(drawpal)		// drawing color palette file path
PARAMDEF(drawfrq)		// drawing color cycling frequency in Hertz
PARAMDEF(nowrap)		// disables automatic word wrapping
PARAMDEF(easing)		// percentage of motion to ease
PARAMDEF(seed)			// starting point for random number generation
PARAMDEF(transtyp)		// transition type override
PARAMDEF(record)		// recording destination folder path
PARAMDEF(recsize)		// recording frame size, in pixels
PARAMDEF(recrate)		// recording frame rate, in frames per second
PARAMDEF(recdur)		// recording duration, in seconds
PARAMDEF(markdown)		// write help markdown to the specified file
PARAMDEF(license)		// display the license

#undef PARAMDEF
#endif

#ifdef HELPEXAMPLEDEF

HELPEXAMPLEDEF(fontsize)
HELPEXAMPLEDEF(fontname)
HELPEXAMPLEDEF(fontwt)
HELPEXAMPLEDEF(transdur)
HELPEXAMPLEDEF(bgclr)
HELPEXAMPLEDEF(bgclr2)
HELPEXAMPLEDEF(bgclr3)
					
#undef HELPEXAMPLEDEF
#endif

#ifdef SLOGANDEF

//			name		member
SLOGANDEF(	text,		sText)
SLOGANDEF(	fontname,	sFontName)
SLOGANDEF(	fontsize,	fFontSize)
SLOGANDEF(	fontwt,		nFontWeight)
SLOGANDEF(	bgclr,		clrBkgnd)
SLOGANDEF(	drawclr,	clrDraw)
SLOGANDEF(	transdur,	fInTransDur)
SLOGANDEF(	holddur,	fHoldDur)
SLOGANDEF(	outdur,		fOutTransDur)
SLOGANDEF(	pausedur,	fPauseDur)
SLOGANDEF(	intrans,	aTransType[TD_INCOMING])
SLOGANDEF(	outtrans,	aTransType[TD_OUTGOING])
					
#undef SLOGANDEF
#endif

#ifdef TRANSTYPEDEF

//				code	name
TRANSTYPEDEF(	SLL,	SCROLL_LR)		// scroll from left to right
TRANSTYPEDEF(	SLR,	SCROLL_RL)		// scroll from right to left
TRANSTYPEDEF(	SLD,	SCROLL_TB)		// scroll from top to bottom
TRANSTYPEDEF(	SLU,	SCROLL_BT)		// scroll from bottom to top
TRANSTYPEDEF(	RVH,	REVEAL_LR)		// reveal or cover from left to right
TRANSTYPEDEF(	RVV,	REVEAL_TB)		// reveal or cover from top to bottom
TRANSTYPEDEF(	TWR,	TYPEWRITER)		// sequentially reveal or cover one letter at a time
TRANSTYPEDEF(	RTW,	RAND_TYPE)		// randomly reveal or cover one letter at a time
TRANSTYPEDEF(	FAD,	FADE)			// fade to or from background color
TRANSTYPEDEF(	SCH,	SCALE_HORZ)		// scale horizontally
TRANSTYPEDEF(	SCV,	SCALE_VERT)		// scale vertically
TRANSTYPEDEF(	SCB,	SCALE_BOTH)		// scale both axes
TRANSTYPEDEF(	SCS,	SCALE_SPIN)		// scale both axes and rotate
TRANSTYPEDEF(	RTL,	RAND_TILE)		// reveal or cover with random tiles
TRANSTYPEDEF(	CVH,	CONVERGE_HORZ)	// converge horizontally
TRANSTYPEDEF(	CVV,	CONVERGE_VERT)	// converge vertically
TRANSTYPEDEF(	MLT,	MELT)			// outline with increasing stroke width
TRANSTYPEDEF(	ELV,	ELEVATOR)		// per-character horizontal reveal
TRANSTYPEDEF(	CLK,	CLOCK)			// per-character radial reveal
TRANSTYPEDEF(	SKW,	SKEW)			// tip over or return to upright
TRANSTYPEDEF(	XPL,	EXPLODE)		// explode each letter into fragments
TRANSTYPEDEF(	SUB,	SUBMARINE)		// rise from or sink into horizon
TRANSTYPEDEF(	TMB,	TUMBLE)			// spin and scale while bunching or spreading
TRANSTYPEDEF(	IRS,	IRIS)			// clip each letter with an ellipse
TRANSTYPEDEF(	BLR,	BLUR)			// blur or sharpen
					
#undef TRANSTYPEDEF
#endif
