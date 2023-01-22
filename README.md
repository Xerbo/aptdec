![Aptdec Logo](textlogo.png)

Copyright (c) 2004-2009 Thierry Leconte (F4DWV), Xerbo (xerbo@protonmail.com) 2019-2022

[![Build](https://github.com/Xerbo/aptdec/actions/workflows/build.yml/badge.svg?branch=master)](https://github.com/Xerbo/aptdec/actions/workflows/build.yml)

## Introduction

Aptdec is a FOSS library/program that decodes images transmitted by the NOAA POES weather satellites. These satellites transmit constantly (among other things) medium resolution (4km/px) images of the earth over a analog mode called APT.
These transmissions can easily be received with a cheap SDR and simple antenna, the transmission can be demodulated in narrow FM mode.

Aptdec can turn the audio into PNG images and generate other products such as:

 - Raw image: both channels (including telemetry)
 - Individual channel: one channel (including telemetry)
 - Visible image: a calibrated visible image of either channel 1 or 2
 - Thermal image: a calibrated thermal image from channel B
 - LUT image: a image where the color is derived from a LUT (used for false color, etc)

The input audio format can be anything supported by `libsndfile` (although only tested with WAV, FLAC and Ogg Vorbis). While sample rate doesn't matter, it is recommended to use 16640 Hz (4x oversampling).

## Quick start

Grab a release from [Releases](https://github.com/Xerbo/aptdec/releases) or compile from source:

```sh
sudo apt install cmake git gcc libsndfile-dev libpng-dev
git clone --recursive https://github.com/Xerbo/aptdec.git && cd aptdec
cmake -B build
cmake --build build
# Resulting binary is build/aptdec-cli
```

In place builds are not supported.

## Examples

To create an image from `gqrx_20200527_115730_137914960.wav` (output filename will be `gqrx_20200527_115730_137914960-raw.png`)
```sh
aptdec-cli gqrx_20200527_115730_137914960.wav
```

To manually specify the output filename
```sh
aptdec-cli -o image.png gqrx_20200527_115730_137914960.wav
```

Decode all WAV files in the current directory:
```sh
aptdec-cli *.wav
```

Apply a denoise filter (see [Post-Processing Effects](#post-processing-effects) for a full list of post-processing effects)
```sh
aptdec-cli -e denoise gqrx_20200527_115730_137914960.wav
```

Create a calibrated IR image from NOAA 18
```sh
aptdec-cli -i thermal gqrx_20200527_115730_137914960.wav
```

Apply a falsecolor LUT
```sh
aptdec-cli -i lut -l luts/WXtoImg-N18-HVC.png gqrx_20200527_115730_137914960.wav
```

## Usage

### Arguments

```
-h, --help                show this help message and exit
-i, --image=<str>         set output image type (see the README for a list)
-e, --effect=<str>        add an effect (see the README for a list)
-s, --satellite=<int>     satellite ID, must be either NORAD or 15/18/19
-l, --lut=<str>           path to a LUT
-o, --output=<str>        path of output image
-r, --realtime            decode in realtime
```

### Image output types

 - `raw`: Raw Image
 - `a`: Channel A (including telemetry)
 - `b`: Channel B (including telemetry)
 - `thermal`: Calibrated thermal (MWIR/LWIR) image
 - `visible`: calibrated visible/NIR image
 - `lut`: LUT image, see also `-l/--lut`

### Post-Processing Effects

 - `strip`: Strip telemetry (only effects raw/a/b images)
 - `equalize`: Histogram equalise
 - `stretch`: Linear equalise
 - `denoise`: Denoise
 - ~~`precipitation`: Precipitation overlay~~
 - `flip`: Flip image (for northbound passes)
 - `crop`: Crop noise from ends of image

## Realtime decoding

Aptdec supports decoding in realtime. The following captures and decodes audio from the `pipewire` interface:

```sh
arecord -f cd -D pipewire | aptdec -r -
```

or directly from an SDR:

```sh
rtl_fm -f 137.1M -g 40 -s 40k | sox -t raw -r 40k -e signed-integer -b 16 - -t wav - | aptdec -r -
```

Image data will be streamed to `CURRENT_TIME.png` (deleted when finished). To stop the decode and normalize the image simply `Ctrl+C` the process.

## LUT format

LUT's are just plain PNG images, 256x256px in size with 24bit RGB color. The X axis represents the value of Channel A and the Y axis the value of Channel B.

## Building for Windows

You can cross build for Windows from Linux (recommended) using with the following commands:
```sh
sudo apt install wget cmake make mingw-w64 git unzip
./build_windows.sh
```

To build natively on Windows using MSVC, you will need: git, ninja and cmake. Then run:
```
./build_windows.bat
```

If you only want to build libaptdec, libpng and libsndfile aren't needed.

## References

- [User's Guide for Building and Operating Environmental Satellite Receiving Stations](https://noaasis.noaa.gov/NOAASIS/pubs/Users_Guide-Building_Receive_Stations_March_2009.pdf)
- [NOAA KLM Users Guide](https://archive.org/details/noaa-klm-guide)

## License

See `LICENSE`
