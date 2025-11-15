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

*/

#ifdef PARAMDEF

PARAMDEF(help)			// display help
PARAMDEF(fullscreen)	// start application in full screen mode
PARAMDEF(fontsize)		// font size in points
PARAMDEF(fontname)		// font name
PARAMDEF(fontweight)	// font weight, from 1 - 999
PARAMDEF(transdur)		// transition duration in seconds
PARAMDEF(holddur)		// hold duration in seconds
PARAMDEF(outdur)		// outgoing transition duration in seconds
PARAMDEF(pausedur)		// pause duration in seconds
PARAMDEF(seqtext)		// display slogans in sequential order instead of randomizing them
PARAMDEF(bgcolor)		// background color in hexadecimal
PARAMDEF(drawcolor)		// drawing color in hexadecimal
PARAMDEF(nowrap)		// disables automatic word wrapping
PARAMDEF(easing)		// percentage of motion to ease
PARAMDEF(seed)			// starting point for random number generation
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
HELPEXAMPLEDEF(fontweight)
HELPEXAMPLEDEF(transdur)
HELPEXAMPLEDEF(bgcolor)
HELPEXAMPLEDEF(drawcolor)
					
#undef HELPEXAMPLEDEF
#endif

