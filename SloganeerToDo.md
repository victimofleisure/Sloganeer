| id | open     | close    | type | description                                                                                             | rev        | user |
|----|----------|----------|------|---------------------------------------------------------------------------------------------------------|------------|------|
| 4  | 11/13/25 | 11/14/25 | done | Add parameter validation                                                                                | 1.0.00.025 |      |
| 6  | 11/13/25 | 11/14/25 | done | Frame size parameter is not implemented; use NxN format                                                 | 1.0.00.025 |      |
| 12 | 11/13/25 | 11/14/25 | done | Verify that parameter color values are actually in RGB and not BGR                                      | 1.0.00.025 |      |
| 1  | 11/13/25 | 11/14/25 | done | Capture should report error if frame size changes                                                       | 1.0.00.025 |      |
| 5  | 11/13/25 | 11/14/25 | done | Test an invalid record path                                                                             | 1.0.00.025 |      |
| 2  | 11/13/25 | 11/14/25 | done | Capture and offscreen classes should have a set format method                                           | 1.0.00.025 |      |
| 13 | 11/13/25 | 11/14/25 | done | Add recording                                                                                           | 1.0.00.025 |      |
| 3  | 11/13/25 | 11/14/25 | done | Record should check for a preexisting image sequence in the destination folder                          | 1.0.00.025 |      |
| 16 | 11/13/25 | 11/14/25 | done | Add help for both command line and GUI; -help and F1                                                    | 1.0.00.026 |      |
| 17 | 11/13/25 | 11/15/25 | done | allow color names as alternative to hexadecimal values                                                  | 1.0.00.027 |      |
| 15 | 11/13/25 | 11/17/25 | done | explode transition, based on tessellation                                                               | 1.0.00.028 |      |
| 20 | 11/17/25 | 11/17/25 | done | Abbreviate overlong command line flags                                                                  | 1.0.00.028 |      |
| 8  | 11/13/25 | 11/20/25 | done | Add CSV file format to allow per-slogan fonts, colors, and transition types                             | 1.0.00.029 |      |
| 9  | 11/13/25 | 11/20/25 | done | CSV file should support assigning a transition type to each slogan, for complete scripting              | 1.0.00.029 |      |
| 10 | 11/13/25 | 11/20/25 | done | For CSV colors, in addition to hexadecimal values, also support HTML color names                        | 1.0.00.029 |      |
| 11 | 11/13/25 | 11/20/25 | done | For CSV transition types, in addition to indices, also support mnemonic codes                           | 1.0.00.029 |      |
| 22 | 11/13/25 | 11/22/25 | done | Add random variant of typewriter transition                                                             | 1.0.00.030 |      |
| 19 | 11/17/25 | 11/23/25 | done | Glyph iterator mishandles certain serif fonts such as Georgia by excluding scraps                       | 1.0.00.031 |      |
| 18 | 11/17/25 | 11/23/25 | done | Glyph iterator handles RTL languages incorrectly, affecting elevator, clock, and explode                | 1.0.00.032 |      |
| 23 | 11/17/25 | 11/24/25 | done | explode transition radiates to left side only for RTL languages                                         | 1.0.00.033 |      |
| 21 | 11/18/25 | 11/26/25 | done | Color cycling though a palette of colors via linear interpolation                                       | 1.0.00.034 |      |
| 25 | 11/26/25 | 11/27/25 | done | Add submarine transition                                                                                | 1.0.00.035 |      |
| 26 | 11/27/25 | 11/27/25 | done | Fix explode transition so triangles radiate from center of glyph's ink box                              | 1.0.00.035 |      |
| 27 | 11/27/25 | 11/27/25 | done | Fix decimal color conversion false positives                                                            | 1.0.00.035 |      |
| 33 | 11/28/25 | 11/29/25 | done | Resizing window during random tiles transition causes incomplete text coverage                          | 1.0.00.038 |      |
| 30 | 11/28/25 | 11/29/25 | done | Reveal leaves scrap in upper left corner; use overhang metrics only in GetTextBounds                    | 1.0.00.038 |      |
| 34 | 11/29/25 | 11/29/25 | done | Refactor simple reveal to use clipping instead of overdrawing in background color                       | 1.0.00.038 |      |
| 7  | 11/13/25 | 12/02/25 | done | Transparent background color breaks some transitions: reveal, random tiles, elevator, clock, fade, melt | 1.0.00.038 |      |
| 32 | 11/28/25 | 12/02/25 | done | Command line parameter to override transition type; useful for testing                                  | 1.0.00.039 |      |
| 29 | 11/28/25 | 12/13/25 | done | add UI for adjusting parameters and adding/editing slogans                                              | 1.0.00.040 |      |
| 24 | 11/26/25 |          | punt | Hollow transition; like melt, but outline in draw color, using glyphs as clipping mask; lacks suspense  |            |      |
| 35 | 11/30/25 |          | punt | Scale up until all glyphs are off screen; way too many degenerate cases                                 |            |      |
| 14 | 11/13/25 |          | todo | Research smoke / fog / blur transition                                                                  |            |      |
| 28 | 11/28/25 |          | todo | Spout support                                                                                           |            |      |
| 31 | 11/28/25 |          | todo | Customize transition probabilities via control file; weighted random list                               |            |      |
| 36 | 12/01/25 |          | todo | Record to named pipe                                                                                    |            |      |
