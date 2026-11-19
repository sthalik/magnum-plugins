Creating input files
====================

`gray.jxl`, `gray16.jxl`, `ga.jxl`, `rgb.jxl`, `rgb16.jxl` and `rgba.jxl` are
lossless conversions of the same-named files in `PngImporter/Test/`, made with
`cjxl` 0.13.0:

```sh
for f in gray gray16 ga rgb rgb16 rgba; do
    cjxl -d 0 -e 11 --allow_expert_options ../../PngImporter/Test/$f.png $f.jxl
done
```

`float.jxl` is a 2x2 grayscale floating-point image with the values 0.0, 0.25,
0.5 and 1.0, converted from a PFM file to EXR with ImageMagick 7 and then to
JPEG XL:

```sh
python -c "
import struct
data = struct.pack('<4f', 0.0, 0.25, 0.5, 1.0)
open('float.pfm', 'wb').write(b'Pf\n2 2\n-1.0\n' + data)
"
magick float.pfm -colorspace Gray -define exr:color-type=Y -depth 16 float.exr
cjxl -d 0 -e 11 --allow_expert_options float.exr float.jxl
```

The remaining files were written with the encoder API of libjxl from Git
(commit `12e4bf40`), with `JxlEncoderAllowExpertOptions()` called and
`JXL_ENC_FRAME_SETTING_EFFORT` set to 11. All except `xyb-icc.jxl` are lossless
(`JxlEncoderSetFrameLossless()`), have `uses_original_profile` set and use the
sRGB color encoding (`JxlColorEncodingSetToSRGB()`):

-   `animated.jxl` is a 2x1 RGB8 animation at 10 ticks per second, looping
    forever, with two frames lasting one tick each. Both pixels of the first
    frame are `{10, 20, 30}`, both pixels of the second are `{200, 100, 50}`.
-   `cmyk.jxl` is a 2x1 RGB8 image with pixels `{10, 20, 30}` and
    `{40, 50, 60}` and an 8-bit `JXL_CHANNEL_BLACK` extra channel with values
    100 and 200, encoded with `JxlEncoderSetCodestreamLevel()` set to 10.
-   `ga-alpha16.jxl` is a 3x1 grayscale image with 8-bit gray and a 16-bit
    alpha channel, encoded from the 16-bit gray and alpha pairs
    `{0x0000, 0x1234}`, `{0x8080, 0xabcd}` and `{0xffff, 0xffff}`.
-   `premultiplied.jxl` is a 2x1 RGBA8 image with `alpha_premultiplied` set
    and both pixels stored as `{10, 20, 40, 51}`.
-   `rgb10.jxl` is a 2x1 RGB image with 10 bits per sample, encoded from the
    16-bit input `{100, 500, 1023}` and `{0, 256, 768}` with
    `JxlEncoderSetFrameBitDepth()` set to `JXL_BIT_DEPTH_FROM_CODESTREAM`, so
    the input values are taken as 10-bit.
-   `rotated.jxl` is a 3x2 RGB8 image with
    `JXL_ORIENT_ROTATE_90_CW` orientation. Its stored rows are
    `{10, 11, 12}, {20, 21, 22}, {30, 31, 32}` and
    `{40, 41, 42}, {50, 51, 52}, {60, 61, 62}`.
-   `xyb-icc.jxl` is an 8x8 RGB8 image with all values 128, encoded lossy with
    `JxlEncoderSetFrameDistance()` set to 1.0 and `uses_original_profile`
    unset, so it's stored in XYB. It has an ICC profile attached with
    `JxlEncoderSetICCProfile()`, made with lcms2's `cmsCreateRGBProfile()`
    with sRGB primaries, a D65 white point and the sRGB curve for green and
    blue. Red gets a parametric curve of type 4 with gamma 3 and
    `a = 0.803409`, `b = 0.196591`, `c = 0.251122`, `d = 0.25`, which matches
    the sRGB curve at 128 and differs from it elsewhere. libjxl replaces a
    profile close to a `JxlColorEncoding` with that encoding and drops the ICC
    data, as it did for lcms2's `cmsCreate_sRGBProfile()` and for a red curve
    of gamma 2.2.
