/*
 *  This file is part of Aptdec.
 *  Copyright (c) 2004-2009 Thierry Leconte (F4DWV), Xerbo (xerbo@protonmail.com) 2019-2020
 *  Copyright (c) 2021 Jon Beniston (M7RCE)
 *
 *  Aptdec is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#ifndef APT_H
#define APT_H

#if defined (__GNUC__) && (__GNUC__ >= 4)
#define APT_API __attribute__((visibility("default")))
#elif defined (_MSC_VER)
#ifdef APT_API_EXPORT
#define APT_API __declspec(dllexport)
#elif APT_API_STATIC
#define APT_API
#else if
#define APT_API __declspec(dllimport)
#endif
#else
#define APT_API
#endif

// Maximum height of an APT image in number of scanlines
#define APT_MAX_HEIGHT 3000

// apt_getpixelrow callback function to get audio samples
typedef int (*apt_getsample_t)(float *samples, int count);

typedef struct {
	float *prow[APT_MAX_HEIGHT]; // Row buffers
	int nrow; // Number of rows
	int zenith; // Row in image where satellite reaches peak elevation
	int chA, chB; // ID of each channel
	char name[256]; // Stripped filename
	char *palette; // Filename of palette
} apt_image_t;

typedef struct {
	float r, g, b;
} apt_rgb_t;

int APT_API apt_init(double sample_rate);
int APT_API apt_getpixelrow(float *pixelv, int nrow, int *zenith, int reset, apt_getsample_t getsample);

void APT_API apt_histogramEqualise(float **prow, int nrow, int offset, int width);
void APT_API apt_linearEnhance(float **prow, int nrow, int offset, int width);
int APT_API apt_calibrate(float **prow, int nrow, int offset, int width) ;
void APT_API apt_denoise(float **prow, int nrow, int offset, int width);
void APT_API apt_flipImage(apt_image_t *img, int width, int offset);
int APT_API apt_cropNoise(apt_image_t *img);

apt_rgb_t APT_API apt_applyPalette(char *palette, int val);
apt_rgb_t APT_API apt_RGBcomposite(apt_rgb_t top, float top_a, apt_rgb_t bottom, float bottom_a);

extern char APT_API apt_TempPalette[256*3];
extern char APT_API apt_PrecipPalette[58*3];

#endif
