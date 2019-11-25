![Aptdec logo](textlogo.png)

Thierry Leconte F4DWV (c) 2004-2009

## Description

Aptdec is an FOSS program that decodes images transmitted by NOAA weather satellites. These satellites transmit continuously (among other things), medium resolution (1px/4km) images of the earth on 137 MHz.  
These transmissions could be easily received with an simple antenna and cheap SDR. Then the transmission can easily be decoded in narrow band FM mode.

Aptdec can convert these audio files into .png images.

For each audio file up to 6 images can be generated:

1. Raw image: contains the 2 transmitted channel images + telemetry and synchronization pulses.
2. Calibrated channel A image
3. Calibrated channel B image
4. Temperature compensated IR image
5. False color image
6. Layered image, boosts cloud visibility

The input audio file must be mono with a sample rate in the range of 4160-62400 Hz, lower samples rates will process faster.
Aptdec uses `libsndfile` to read the input audio, so any format supported by `libsndfile` may be used (however it has only tested with `.wav` files).

## Compilation

Aptdec is portable since it is written in standard C.
It has successfully compiled and ran on Debian with both `gcc` and `clang` and will most likely work on any Unix platform.
Just edit the Makefile and run `make` (no configure script as of right now). 

Aptdec uses `libsndfile`, `libpng` and `libm`.
The `snd.h` and `png.h` headers must be present on your system.
If they are not on standard path, edit the include path in the Makefile.

## Usage

To compile  
`make`

To run without installing  
`./aptdec [options] audio files...`

To install  
`sudo make install`

To run once installed  
`aptdec [options] audio files...`

To uninstall  
`sudo make uninstall`

## Options

```
-i [r|a|b|c|t|l]
Output image type
Raw (r), Channel A (a), Channel B (b), False Color (c), Temperature (t), Layered (l)
Default: "ab"

-d <dir>
Images destination directory (optional).
Default: Current directory

-s [15|16|17|18|19]
Satellite number
For temperature calibration
Default: "19"

-e [c|t]
Enhancements
Contrast (c) or Crop Telemetry (t)
Defaults: "ct"

-c <file>
Use configuration file for false color generation.
Default: Internal defaults
```

## Output

Generated images are outputted in PNG, 8 bit greyscale for raw and channel A|B images, 24 bit RGB for false color.

Image names are `audiofile-x.png`, where `x` is:

 - `r` for raw images
 - Sensor ID (1, 2, 3A, 3B, 4, 5) for channel A|B images
 - `c` for false color.
 - `t` for temperature calibrated images
 - `l` for layered images

Currently there are 2 available enchancements:

 - `c` for contrast enhancements, on by default
 - `t` for crop telemetry, on by default, only has effects on raw images

## Example

`aptdec -d images -i ab *.wav`

This will process all `.wav` files in the current directory, generate contrast enhanced channel A and B images and put them in the `images` directory.

## Further reading

[https://noaasis.noaa.gov/NOAASIS/pubs/Users_Guide-Building_Receive_Stations_March_2009.pdf](https://noaasis.noaa.gov/NOAASIS/pubs/Users_Guide-Building_Receive_Stations_March_2009.pdf)  
[https://web.archive.org/web/20141220021557/https://www.ncdc.noaa.gov/oa/pod-guide/ncdc/docs/klm/tables.htm](https://web.archive.org/web/20141220021557/https://www.ncdc.noaa.gov/oa/pod-guide/ncdc/docs/klm/tables.htm)

## License

See LICENSE.