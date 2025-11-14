# Sloganeer

## Lightweight signage app that displays text with animated transitions.

# Command line

### Usage: Sloganeer slogans_path [options]

Simple example: `Sloganeer myslogans.txt -fontname Calibri -fontsize 120`

**Numeric parameters accept fractional values unless specified otherwise.**

|Option|Description|
|---|---|
|slogans_path|The path of an ANSI or UTF-8 text file containing the slogans to display, one slogan per line; if the path contains spaces, enclose it in double quotes.|
|&#8209;fullscreen|Start the application in full screen mode.|
|&#8209;fontsize&nbsp;SIZE|Font size in points.|
|&#8209;fontname&nbsp;NAME|Font name; if the font name contains spaces, enclose it in double quotes.|
|&#8209;fontweight&nbsp;WEIGHT|Font weight from 1 to 999. Integer only.|
|&#8209;transdur&nbsp;SECS|Transition duration in seconds. Applies to both incoming and outgoing transitions unless the outdur option is also specified.|
|&#8209;holddur&nbsp;SECS|Duration for which slogan is immobile, in seconds.|
|&#8209;outdur&nbsp;SECS|Outgoing transition duration in seconds. Only needed if outgoing transitions should have a different duration than incoming transitions.|
|&#8209;pausedur&nbsp;SECS|Duration of pause between slogans, in seconds.|
|&#8209;seqtext|Display slogans sequentially instead of shuffling them.|
|&#8209;bgcolor&nbsp;COLOR|Background color in hexadecimal.|
|&#8209;drawcolor&nbsp;COLOR|Drawing color in hexadecimal.|
|&#8209;nowrap|Disables automatic word wrapping.|
|&#8209;easing PCT|The percentage of motion to ease. Only applies to transition types that support easing.|
|&#8209;record PATH|Destination folder for recording an image sequence.|
|&#8209;recsize SIZE|Recording frame width and height in pixels, separated by a lowercase 'x' as in 640x480.|
|&#8209;recrate FPS|Recording frame rate in frames per second.|
|&#8209;recdur SECS|Recording duration in seconds.|

### Examples

|Example|Description|
|---|---|
|&#8209;fontsize&nbsp;90|Set the font size to 90 points.|
|&#8209;fontname&nbsp;"Times New Roman"|Set the font name to Times New Roman.|
|&#8209;fontweight&nbsp;400|Set the font weight to 400.|
|&#8209;transdur&nbsp;2.5|Set the transition duration to 2.5 seconds.|
|&#8209;bgcolor&nbsp;A9A9A9|Set the background color to dark gray.|

Each slogan cycles through the following four states:

1. Incoming transition
2. Hold
3. Outgoing transition
4. Pause

The duration of each of these states can be set independently. To skip a state, give it a duration of zero.

Note that if you only specify the transdur option, it applies to both incoming and outgoing transitions. To make the outgoing transition a different duration from the incoming transition, you must also specify the outdur option.

For example, to set incoming transitions to 1 second, a hold time of 3 seconds, no outgoing transition, and a pause of 1.5 seconds:

`-transdur 1 -holddur 3 -outdur 0 -pausedur 1.5`

### Slogans file

The slogans file is a plain text file containing the slogans to be displayed. Each non-empty line of the file is interpreted as a single slogan. You can create multi-line slogans by inserting tab characters where you want line breaks. Each tab in a slogan is replaced with a newline when the text is displayed. To create a blank slogan, use a line containing a single space.

The slogans file must be in either ANSI or UTF-8 format. To use non-ANSI characters, UTF-8 format is strongly recommended. UTF-8 supports nearly all languages including Chinese, Japanese, Arabic, and Hindi.

### Transitions

1. Scroll from left to right
2. Scroll from right to left
3. Scroll from top to bottom
4. Scroll from bottom to top
5. Reveal or cover from left to right
6. Reveal or cover from top to bottom
7. Reveal or cover one letter at a time
8. Fade to or from background color
9. Scale horizontally
10. Scale vertically
11. Scale both axes
12. Scale both axes and rotate
13. Reveal or cover with tiles
14. Converge horizontally
15. Converge vertically
16. Melt or grow by erasing outline
17. Per-character horizontal reveal
18. Per-character radial reveal
19. Tip over or return to upright

### Defaults

Randomized slogan order, white text on black background, 150 point Arial Black (weight 900). Each transition takes two seconds, with one second of hold time, and no pause between slogans. Easing is 15% for transitions that support it. Recording defaults to 1920x1080, 60 FPS, one minute.

### Shortcuts

To enter or exit full screen mode, use the app's system menu or the shortcut key F11.

### Installation

There is no installer. Unzip the distribution file and run the executable. The application does not store settings in the registry or anywhere else.

### Development

There's a [development blog](https://sloganeers.blogspot.com/). See you there!
