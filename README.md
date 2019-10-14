# Aptdec

Thierry Leconte F4DWV (c) 2004-2009

## Description

Aptec is an FOSS program that decodes images transmitted by POES NOAA weather satellites.  
These satellites transmit continuously (among other things), medium resolution (1px/4km) images of the earth on 137 MHz.  
These transmissions could be easily received with an simple antenna and cheap SDR.  
Then the transimssion can easily be decoded in narrow band FM mode.

Aptdec can convert these recordings into .png images.

For each recording up to 6 images can be generated:

1. Raw image: contains the 2 transmitted channel images + telemetry 
and synchronisation pulses.
2. Calibrated channel A image
3. Calibrated channel B image
4. Temperature compensated I.R image
5. False color image

Input recordings must be mono with a sample rate of 11025 Hz.
Aptdec uses `libsndfile` to read the input recording, so any format supported by `libsndfile` may be used (only tested with .wav files).

## Compilation

Aptdec is written is portable since it is written in standard C.
It has only tested on Fedora and Debian, but will work on any Unix platform.
Just edit the Makefile and run `make` (no configure script as of now). 

Aptdec uses `libsndfile`, `libpng` and `libm`.
The `snd.h` and `png.h` headers must be present on your system.
If they are not on standard path, edit the include path in the Makefile.

## Usage

To run without installing  
`./Aptdec [options] recordings ...`

To install  
`sudo make install`

To uninstall  
`sudo make uninstall`

To run once installed  
`Aptdec [options] recordings ...`

## Options

```
-i [r|a|b|c|t]
Toggle raw (r), channel A (a), channel B (b), false color (c),
or temperature (t) output.
Default: ac

-d <dir>
Optional images destination directory.
Default: Recording directory.

-s [15|16|17|18|19]
Satellite number
For temperature and false color generation
Default: 19

-c <file>
Use configuration file for false color generation.
Default: Internal parameters.
```

## Output

Generated images are outputted in PNG, 8 bit greyscale for raw and channel A|B images, 24 bit RGB for false color.

Image names are `recordingname-x.png`, where `x` is:

 - r for raw images  
 - satellite instrument number (1, 2, 3A, 3B, 4, 5) for channel A|B images  
 - c for false colors.  

## Example

`Aptdec -d images -i ac *.wav`

This will process all .wav files in the current directory, generate channel A and false color images and put them in the `images` directory.

## License

See LICENSE.