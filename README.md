# Sloganeer

## Lightweight signage app that displays text with animated transitions.

# Command line

### Usage: sloganeer slogans_path [options]

|Option|Description|
|---|---|
|slogans_path|The path of an ANSI or UTF-8 text file containing the slogans to display, one slogan per line; if the path contains spaces, enclose it in double quotes.|
|&#8209;fullscreen|Start the application in full screen mode.|
|&#8209;fontsize&nbsp;SIZE|Font size in points.|
|&#8209;fontname&nbsp;NAME|Font name; if the path contains spaces, enclose it in double quotes.|
|&#8209;fontweight&nbsp;WEIGHT|Font weight from 1 to 999.|
|&#8209;transdur&nbsp;SECS|Transition duration in seconds (fractions allowed).|
|&#8209;holddur&nbsp;SECS|Hold duration in seconds (fractions allowed).|
|&#8209;seqtext|Display slogans in sequential order instead of randomizing them.|

### Examples

|Example|Description|
|---|---|
|&#8209;fontsize&nbsp;90|Set the font size to 90 points.|
|&#8209;fontname&nbsp;"Times New Roman"|Set the font name to Times New Roman.|
|&#8209;fontweight&nbsp;400|Set the font weight to 400.|
|&#8209;transdur&nbsp;2.5|Set the transition duration to 2.5 seconds.|


