# Sloganeer

## Artisanal signage app that displays text with animated transitions.

### Command line

Simple example: `Sloganeer myslogans.txt -fontname Calibri -fontsize 120`

**Numeric parameters accept fractional values unless specified otherwise.**

### Usage: Sloganeer SLOGANS_PATH [options]

|Option|Description|
|---|---|
|SLOGANS_PATH|The path of an ANSI or UTF-8 text file containing the slogans to display, one slogan per line; if the path contains spaces, enclose it in double quotes.|
|&#8209;help|Show help; -h is also accepted.|
|&#8209;fullscr|Start the application in full screen mode.|
|&#8209;fontsize&nbsp;SIZE|Font size in points.|
|&#8209;fontname&nbsp;NAME|Font name; if the font name contains spaces, enclose it in double quotes.|
|&#8209;fontwt&nbsp;WEIGHT|Font weight from 1 to 999; integer only.|
|&#8209;transdur&nbsp;SECS|Transition duration in seconds. Applies to both incoming and outgoing transitions unless the outdur option is also specified.|
|&#8209;holddur&nbsp;SECS|Duration for which slogan is immobile, in seconds.|
|&#8209;outdur&nbsp;SECS|Outgoing transition duration in seconds. Only needed if outgoing transitions should have a different duration than incoming transitions.|
|&#8209;pausedur&nbsp;SECS|Duration of pause between slogans, in seconds.|
|&#8209;seqtext|Display slogans sequentially instead of shuffling them.|
|&#8209;bgclr&nbsp;COLOR|Background color as a hexadecimal value, comma-separated decimal values, or an HTML color name.|
|&#8209;bgpal&nbsp;PATH|Path to a background color palette: a text file with one color per line, each consisting of a hexadecimal value, comma-separated decimal values, or an HTML color name.|
|&#8209;bgfrq&nbsp;FREQ|Background color cycling frequency in Hertz; only effective if a background color palette is specified.|
|&#8209;drawclr&nbsp;COLOR|Drawing color as a hexadecimal value, comma-separated decimal values, or an HTML color name.|
|&#8209;drawpal&nbsp;PATH|Path to a drawing color palette: a text file with one color per line, each consisting of a hexadecimal value, comma-separated decimal values, or an HTML color name.|
|&#8209;drawfrq&nbsp;FREQ|Drawing color cycling frequency in Hertz; only effective if a drawing color palette is specified.|
|&#8209;nowrap|Disables automatic word wrapping.|
|&#8209;easing&nbsp;PCT|The percentage of motion to ease. Only applies to transition types that support easing.|
|&#8209;seed|Starting point for random number generation; integer only. If not specified, seed is current time so that each run is different.|
|&#8209;record&nbsp;PATH|Destination folder path for recording an image sequence.|
|&#8209;recsize&nbsp;SIZE|Recording frame width and height in pixels, separated by a lowercase 'x' as in 640x480.|
|&#8209;recrate&nbsp;FPS|Recording frame rate in frames per second.|
|&#8209;recdur&nbsp;SECS|Recording duration in seconds.|
|&#8209;markdown&nbsp;PATH|Write help markdown to the specified file.|
|&#8209;license|Display the license.|

### Examples

|Example|Description|
|---|---|
|&#8209;fontsize&nbsp;90|Set the font size to 90 points.|
|&#8209;fontname&nbsp;Georgia|Set the font name to Georgia.|
|&#8209;fontwt&nbsp;400|Set the font weight to 400.|
|&#8209;transdur&nbsp;2.5|Set the transition duration to 2.5 seconds.|
|&#8209;bgclr&nbsp;FF0000|Set the background color to red.|
|&#8209;bgclr&nbsp;255,0,0|Set the background color to red.|
|&#8209;bgclr&nbsp;red|Set the background color to red.|

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

### CSV slogans file

Sloganeer supports an alternate file format that lets you customize the properties of each slogan, including its font, colors, timing and transition types. It’s a well-known format called comma-separated values, also known as CSV. The CSV file can be ANSI or UTF-8, but UTF-8 is strongly recommended for languages other than English.

CSV format supports the columns listed below. Most of these columns have the same name and meaning as a command line option, so they’re only described briefly here; refer to the [command line](#command-line) reference for additional information.

|Column|Description|
|-|-|
|text|slogan text|
|fontname|font name|
|fontsize|font size in points|
|fontwt|font weight, from 1 - 999|
|bgclr|background color|
|drawclr|drawing color|
|transdur|transition duration in seconds|
|holddur|hold duration in seconds|
|outdur|outgoing transition duration in seconds|
|pausedur|pause duration in seconds|
|intrans|incoming transition type|
|outtrans|outgoing transition type|

If a column is omitted, the value is obtained from the corresponding command line option, or if none exists, from an application default.

Note that if you only specify the transdur column, it applies to both incoming and outgoing transitions. To make the outgoing transition a different duration from the incoming transition, you must also specify the outdur column.

A transition type can be specified as a zero-based index or as a mnemonic transition code. The codes are preferable as they’re easier to remember and indices may change. The codes are given in the table of supported [transitions](#transitions) elsewhere in this document. Transition codes are case-insensitive.

The CSV can optionally start with a header row, indicating which columns are specified. This lets you specify a noncontiguous subset of the columns without typing lots of commas, and also lets you specify the columns in any order. Some examples follow.

No header row, all columns specified:

```
My Slogan,Verdana,100,400,Green,Red,2,4,3,1,mlt,clk
```

Displays the text “My Slogan” in 100-point Verdana, normal weight, drawn red on a green background. The timing is 2 seconds incoming, 4 seconds hold, 3 seconds outgoing, and 1 second pause. The transition types are Melt incoming, and Clock outgoing.

No header row, specifying a subset of columns (text, fontname, drawclr):

```
My Slogan,Verdana,,,,Red
```

Displays the text “My Slogan” in Verdana, drawn red. The commas are necessary in order to skip fontsize, fontwt, and bgclr. The commas could be avoided by using a header row:

```
text,fontname,drawclr
My Slogan,Verdana,Red
```

If your slogan text contains commas, you must enclose the entire slogan in double quotes:

```
"My slogan, has a comma"
```

And if your slogan also contains double quotes, you must double them:

```
"My ""slogan"", has a comma"
```

The slogan text also supports a few common escape sequences:

|Seq|Meaning|
|-|-|
|\\n |newline|
|\\t |tab|
|\\\\ |backslash|

For example, this slogan forces a line break between “My Slogan” and “has two lines”:

```
My slogan\nhas two lines
```

### Transitions

|#|Code|Name|Description|
|-|-|-|-|
|1|SLL|Scroll LR|Scroll from left to right|
|2|SLR|Scroll RL|Scroll from right to left|
|3|SLD|Scroll TB|Scroll from top to bottom|
|4|SLU|Scroll BT|Scroll from bottom to top|
|5|RVH|Reveal LR|Reveal or cover from left to right|
|6|RVV|Reveal TB|Reveal or cover from top to bottom|
|7|TWR|Typewriter|Sequentially reveal or cover one letter at a time|
|8|RTW|Rand Type|Randomly reveal or cover one letter at a time|
|9|FAD|Fade|Fade to or from background color|
|10|SCH|Scale Horz|Scale horizontally|
|11|SCV|Scale Vert|Scale vertically|
|12|SCB|Scale Both|Scale both axes|
|13|SCS|Scale Spin|Scale both axes and rotate|
|14|RTL|Rand Tile|Reveal or cover with random tiles|
|15|CVH|Converge H|Converge horizontally|
|16|CVV|Converge V|Converge vertically|
|17|MLT|Melt|Melt or grow by erasing outline|
|18|ELV|Elevator|Per-character horizontal reveal|
|19|CLK|Clock|Per-character radial reveal|
|20|SKW|Skew|Tip over or return to upright|
|21|XPL|Explode|Explode each letter into fragments|

### Colors

Colors can be specified in three ways.

1. Hexadecimal color value, consisting of up to eight characters, each of which is [0-9] or [A-F]. The color channels are expected to be in RGB or RGBA order. If six or less characters are specified, RGB is assumed and the alpha channel defaults to opaque. The value is case-insensitive. Do not prefix the value with a pound sign. Example (for Crimson): `DC143C`

2. Decimal color value, consisting of three or four decimal numbers, each of which is [0-255], separated by commas and/or spaces. For colors specified via command line, commas are easier, as to use spaces in a command line parameter, you must enclose the parameter in double quotes. For CSV files, spaces are easier, as to use commas in a CSV value, you must enclose the value in double quotes. The color channels are expected to be in RGB or RGBA order. If three numbers are specified, RGB is assumed and the alpha channel defaults to opaque. Fractional values are accepted. Example (for Crimson): `220,20,60`

3. HTML color name. The name is must be one of the 140 defined [color names](https://www.w3schools.com/tags/ref_colornames.asp). Color names are case-insensitive. Example (for Crimson): `Crimson`

### Defaults

Randomized slogan order, white text on black background, 150 point Arial Black (weight 900). Each transition takes two seconds, with one second of hold time, and no pause between slogans. Easing is 15% for transitions that support it. Recording defaults to 1920x1080, 60 FPS, one minute.

### Shortcuts

To enter or exit full screen mode, use the app’s system menu or the shortcut key F11.

### Installation

There is no installer. Unzip the distribution file and run the executable. The application does not store settings in the registry or anywhere else.

### Development

* There’s a [development blog](https://sloganeers.blogspot.com/). See you there!
* The issues list is [here](SloganeerToDo.md).
