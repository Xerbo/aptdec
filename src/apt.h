/*
 *  This file is part of Aptdec.
 *  Copyright (c) 2004-2009 Thierry Leconte (F4DWV), Xerbo (xerbo@protonmail.com) 2019-2022
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

#ifdef __cplusplus
extern "C" {
#endif

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

// Maximum height of an APT image in number of rows
#define APT_MAX_HEIGHT 3000
// Width in pixels of sync
#define APT_SYNC_WIDTH 39
// Width in pixels of space
#define APT_SPC_WIDTH  47
// Width in pixels of telemetry
#define APT_TELE_WIDTH 45
// Width in pixels of a single channel image
#define APT_CH_WIDTH   909
#define APT_FRAME_LEN  128
#define APT_CH_OFFSET  (APT_SYNC_WIDTH+APT_SPC_WIDTH+APT_CH_WIDTH+APT_TELE_WIDTH)
// Width in pixels of full frame, including sync, space, images and telemetry
#define APT_IMG_WIDTH  2080
// Offset in pixels to channel A
#define APT_CHA_OFFSET (APT_SYNC_WIDTH+APT_SPC_WIDTH)
// Offset in pixels to channel B
#define APT_CHB_OFFSET (APT_SYNC_WIDTH+APT_SPC_WIDTH+APT_CH_WIDTH+APT_TELE_WIDTH+APT_SYNC_WIDTH+APT_SPC_WIDTH)
#define APT_TOTAL_TELE (APT_SYNC_WIDTH+APT_SPC_WIDTH+APT_TELE_WIDTH+APT_SYNC_WIDTH+APT_SPC_WIDTH+APT_TELE_WIDTH)

// Number of rows required for apt_calibrate
#define APT_CALIBRATION_ROWS 192
// Channel ID returned by apt_calibrate
// NOAA-15: https://nssdc.gsfc.nasa.gov/nmc/experiment/display.action?id=1998-030A-01
//  Channel 1:  visible (0.58-0.68 um)
//  Channel 2:  near-IR (0.725-1.0 um)
//  Channel 3A: near-IR (1.58-1.64 um)
//  Channel 3B: mid-infrared (3.55-3.93 um)
//  Channel 4:  thermal-infrared (10.3-11.3 um)
//  Channel 5:  thermal-infrared (11.5-12.5 um)
typedef enum apt_channel {APT_CHANNEL_UNKNOWN, APT_CHANNEL_1, APT_CHANNEL_2, APT_CHANNEL_3A, APT_CHANNEL_4, APT_CHANNEL_5, APT_CHANNEL_3B} apt_channel_t;

// Width in elements of apt_image_t.prow arrays
#define APT_PROW_WIDTH 2150

// apt_getpixelrow callback function to get audio samples.
// context is the same as passed to apt_getpixelrow.
typedef int (*apt_getsamples_t)(void *context, float *samples, int count);

typedef struct {
	float *prow[APT_MAX_HEIGHT]; // Row buffers
	int nrow; // Number of rows
	apt_channel_t chA, chB; // ID of each channel
	char name[256]; // Stripped filename
	char *palette; // Filename of palette
} apt_image_t;

typedef struct {
	float r, g, b;
} apt_rgb_t;

int APT_API apt_init(double sample_rate);
int APT_API apt_getpixelrow(float *pixelv, int nrow, int *zenith, int reset, apt_getsamples_t getsamples, void *context);

void APT_API apt_histogramEqualise(float **prow, int nrow, int offset, int width);
void APT_API apt_linearEnhance(float **prow, int nrow, int offset, int width);
apt_channel_t APT_API apt_calibrate(float **prow, int nrow, int offset, int width) ;
void APT_API apt_denoise(float **prow, int nrow, int offset, int width);
void APT_API apt_flipImage(apt_image_t *img, int width, int offset);
int APT_API apt_cropNoise(apt_image_t *img);
void APT_API apt_calibrate_thermal(int satnum, apt_image_t *img, int offset, int width);
void APT_API apt_calibrate_visible(int satnum, apt_image_t *img, int offset, int width);
// Moved to apt_calibrate_thermal
#define apt_temperature apt_calibrate_thermal

apt_rgb_t APT_API apt_applyPalette(char *palette, int val);
apt_rgb_t APT_API apt_RGBcomposite(apt_rgb_t top, float top_a, apt_rgb_t bottom, float bottom_a);

extern char APT_API apt_TempPalette[256*3];
extern char APT_API apt_PrecipPalette[58*3];

#ifdef __cplusplus
}
#endif

#endif
