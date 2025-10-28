# Sloganeer

## Lightweight signage app that displays text with animated transitions.

# Command line

### Usage: sloganeer slogans_path [options]

|Option|Description|
|---|---|
|slogans_path|The path of an ANSI or UTF-8 text file containing the slogans to display, one slogan per line; if the path contains spaces, enclose it in double quotes.|
|-fullscreen||Start the application in full screen mode.|
|-fontsize SIZE||Font size in points.|
|-fontname NAME||Font name; if the path contains spaces, enclose it in double quotes.|
|-fontweight WEIGHT||Font weight from 1 to 999.|
|-transdur SECS||Transition duration in seconds (fractions allowed).|
|-holddur SECS||Hold duration in seconds (fractions allowed).|
|-seqtext||Display slogans in sequential order instead of randomizing them.|

