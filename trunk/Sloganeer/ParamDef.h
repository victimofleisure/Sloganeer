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

*/

#ifdef PARAMDEF

PARAMDEF(fullscreen)	// start application in full screen mode
PARAMDEF(fontsize)		// font size in points
PARAMDEF(fontname)		// font name
PARAMDEF(fontweight)	// font weight from 1 - 999
PARAMDEF(transdur)		// transition duration in seconds (fractions allowed)
PARAMDEF(outdur)		// outgoing transition duration in seconds (fractions allowed)
PARAMDEF(holddur)		// hold duration in seconds (fractions allowed)
PARAMDEF(pausedur)		// pause duration in seconds (fractions allowed)
PARAMDEF(seqtext)		// display slogans in sequential order instead of randomizing them
PARAMDEF(bgcolor)		// background color in hexadecimal
PARAMDEF(drawcolor)		// drawing color in hexadecimal
PARAMDEF(nowrap)		// disables automatic word wrapping

#undef PARAMDEF
#endif
