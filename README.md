# Sloganeer

## Lightweight signage app that displays text with animated transitions.

# Command line

### Usage: sloganeer slogans_path [options]

|Option|Description|
|---|---|
|slogans_path|The path of an ANSI or UTF-8 text file containing the slogans to display, one slogan per line; if the path contains spaces, enclose it in double quotes.|
|&#8209;fullscreen|Start the application in full screen mode.|
|&#8209;fontsize&nbsp;SIZE|Font size in points.|
|&#8209;fontname&nbsp;NAME|Font name; if the font name contains spaces, enclose it in double quotes.|
|&#8209;fontweight&nbsp;WEIGHT|Font weight from 1 to 999.|
|&#8209;transdur&nbsp;SECS|Transition duration in seconds; fractions of a second are allowed.|
|&#8209;holddur&nbsp;SECS|Duration for which slogan is immobile, in seconds; fractions of a second are allowed.|
|&#8209;pausedur&nbsp;SECS|Duration of pause between slogans, in seconds; fractions of a second are allowed.|
|&#8209;seqtext|Display slogans in sequential order instead of randomizing them.|
|&#8209;bgcolor&nbsp;COLOR|Background color in hexadecimal.|
|&#8209;drawcolor&nbsp;COLOR|Drawing color in hexadecimal.|

### Examples

|Example|Description|
|---|---|
|&#8209;fontsize&nbsp;90|Set the font size to 90 points.|
|&#8209;fontname&nbsp;"Times New Roman"|Set the font name to Times New Roman.|
|&#8209;fontweight&nbsp;400|Set the font weight to 400.|
|&#8209;transdur&nbsp;2.5|Set the transition duration to 2.5 seconds.|
|&#8209;bgcolor&nbsp;A9A9A9|Set the background color to dark gray.|

Sample usage: Sloganeer myslogans.txt -seqtext -fontname Calibri -fontweight 400

### Slogans file

The slogans file is a plain text file containing the slogans to be displayed. Each non-blank line of the file is interpreted as a single slogan. You can create multi-line slogans by inserting tab characters where you want line breaks. Each tab in a slogan is replaced with a newline when the text is displayed.

The slogans file must be in either ANSI or UTF-8 format. To use non-ANSI characters, UTF-8 format is strongly recommended. UTF-8 supports nearly all languages including Chinese, Japanese, Arabic, and Hindi.

### Transitions

* Scroll from left to right
* Scroll from right to left
* Scroll from top to bottom
* Scroll from bottom to top
* Reveal or cover from left to right
* Reveal or cover from top to bottom
* Reveal or cover one letter at a time
* Fade to or from background color
* Scale horizontally
* Scale vertically
* Scale both axes
* Reveal or cover with tiles

### Defaults

Randomized text order, white text on black background, 150 point Arial Black 
(weight 900), five second hold time, and no pause between slogans.

### Installation

There is no installer. Unzip the distribution file and run the executable.
The application does not store settings in the registry or anywhere else.
