**Regression Test Reference Images**

These TIFF files serve as Sloganeer's regression-test baselines.

They were generated using 120-point Times New Roman Black (weight 900).

The test slogan has two lines: an English phrase ("Your Text"), and the
Arabic phrase for "Hello World." These phrases were chosen specifically
to exercise mixed-direction text rendering, glyph metrics edge cases,
and DirectWrite's bidi behavior.

**How to run the regression test**

To enable regression testing in Sloganeer:

1. In SloganDraw.h, set SD_CAPTURE to SD_CAPTURE_REGRESSION_TEST
instead of SD_CAPTURE_NONE.

2. In SloganDraw.cpp, inside the RegressionTest method, set the
sRefFolder string to the path of this tests\regression\tnr
folder on your machine.

3. Recompile and run Sloganeer.

When regression test mode is active, Sloganeer renders every transition
type in both directions, captures the resulting images, and performs a
binary comparison against these reference TIFF files. After completing
the full matrix of tests, Sloganeer displays a pass/fail summary in a
message box.

These images are part of the renderer's specification. Any difference in
output should be considered a regression unless you explicitly regenerate
and update the baseline images.

**Generating new reference images**

To regenerate the baseline TIFFs:

In SloganDraw.h, set SD_CAPTURE to SD_CAPTURE_MAKE_REF_IMAGES. This
causes Sloganeer to render the full regression set and save new reference images
instead of comparing against existing ones.

You may also set SD_CAPTURE to SD_CAPTURE_RECORD, which captures whatever
Sloganeer displays. Unlike the user-facing recording mode, this internal capture
path grabs frames directly from the swap chain, bypassing the offscreen render
target. This mode is intended strictly for debugging and should never be enabled
in a Release build.

Swap-chain capture supports full-screen mode, provided full-screen was selected
via command-line option. Resizing the window during capture is not supported and
will trigger an error.
