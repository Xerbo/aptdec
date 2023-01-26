/*
 * aptdec - A lightweight FOSS (NOAA) APT decoder
 * Copyright (C) 2004-2009 Thierry Leconte (F4DWV) 2019-2023 Xerbo (xerbo@protonmail.com)
 * Copyright (C) 2021 Jon Beniston (M7RCE)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef APTDEC_H_
#define APTDEC_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__GNUC__) && (__GNUC__ >= 4)
# define APTDEC_API __attribute__((visibility("default")))
#elif defined(_MSC_VER)
# ifdef APTDEC_API_EXPORT
#  define APTDEC_API __declspec(dllexport)
# else
#  define APTDEC_API __declspec(dllimport)
# endif
#else
# define APTDEC_API
#endif

// Height of a single telemetry wedge
#define APTDEC_WEDGE_HEIGHT 8
// Numbers of wedges in a frame
#define APTDEC_FRAME_WEDGES 16
// Height of a telemetry frame
#define APTDEC_FRAME_LEN (APTDEC_WEDGE_HEIGHT * APTDEC_FRAME_WEDGES)

// Width of the overall image
#define APTDEC_IMG_WIDTH 2080
// Width of sync marker
#define APTDEC_SYNC_WIDTH 39
// Width of space view
#define APTDEC_SPC_WIDTH 47
// Width of telemetry
#define APTDEC_TELEMETRY_WIDTH 45
// Width of a single video channel
#define APTDEC_CH_WIDTH 909

// Offset to channel A video data
#define APTDEC_CHA_OFFSET (APTDEC_SYNC_WIDTH + APTDEC_SPC_WIDTH)
// Offset to channel B video data
#define APTDEC_CHB_OFFSET (APTDEC_SYNC_WIDTH + APTDEC_SPC_WIDTH + APTDEC_CH_WIDTH + APTDEC_TELEMETRY_WIDTH + APTDEC_SYNC_WIDTH + APTDEC_SPC_WIDTH)

// Number of rows needed for aptdec_normalize to (reliably) work
#define APTDEC_NORMALIZE_ROWS (APTDEC_FRAME_LEN * 2)
// Maximum amount of samples that will be requested from aptdec_callback_t
#define APTDEC_BUFFER_SIZE 16384

// Channel 1:  visible (0.58-0.68 um)
// Channel 2:  near-IR (0.725-1.0 um)
// Channel 3A: near-IR (1.58-1.64 um)
// Channel 3B: mid-infrared (3.55-3.93 um)
// Channel 4:  thermal-infrared (10.3-11.3 um)
// Channel 5:  thermal-infrared (11.5-12.5 um)
typedef enum aptdec_channel {
    AVHRR_CHANNEL_UNKNOWN,
    AVHRR_CHANNEL_1,
    AVHRR_CHANNEL_2,
    AVHRR_CHANNEL_3A,
    AVHRR_CHANNEL_4,
    AVHRR_CHANNEL_5,
    AVHRR_CHANNEL_3B
} avhrr_channel_t;

typedef enum aptdec_satellite {
    NOAA15,
    NOAA18,
    NOAA19
} aptdec_satellite_t;

typedef struct aptdec_image {
    uint8_t *data;  // Image data
    size_t rows;    // Number of rows

    // Telemetry
    aptdec_satellite_t satellite;
    avhrr_channel_t ch[2];
    float space_view[2];
    float telemetry[2][16];
} aptdec_image_t;

typedef struct aptdec_rgb {
    uint8_t r, g, b;
} aptdec_rgb_t;

typedef struct aptdec_region {
    size_t offset;
    size_t width;
} aptdec_region_t;

typedef struct aptdec aptdec_t;

// Callback function to get samples
// `context` is the same as passed to aptdec_getrow
typedef size_t (*aptdec_callback_t)(float *samples, size_t count, void *context);

// Clone an aptdec_image_t struct
// Useful for calibration
APTDEC_API aptdec_image_t aptdec_image_clone(aptdec_image_t img);

// Returns version of libaptdec in git tag format
// i.e. v2.0.0 or v2.0.0-1-xxxxxx
APTDEC_API char *aptdec_get_version(void);

// Create and destroy libaptdec instances
// If aptdec_init fails it will return NULL
APTDEC_API aptdec_t *aptdec_init(float sample_rate);
APTDEC_API void aptdec_free(aptdec_t *aptdec);

// Normalize and quantize raw image data
// Data is arranged so that each row starts at APTDEC_IMG_WIDTH*y
APTDEC_API aptdec_image_t aptdec_normalize(const float *data, size_t rows, aptdec_satellite_t satellite, int *error);

// Get an entire row of pixels
// Requires that `row` has enough space to store APTDEC_IMG_WIDTH*2
// Returns 0 when `callback` return value != count
APTDEC_API int aptdec_getrow(aptdec_t *aptdec, float *row, aptdec_callback_t callback, void *context);

// Calibrate channels
APTDEC_API int aptdec_calibrate_thermal(aptdec_image_t *img, aptdec_region_t region);
APTDEC_API int aptdec_calibrate_visible(aptdec_image_t *img, aptdec_region_t region);

APTDEC_API void aptdec_denoise (aptdec_image_t *img, aptdec_region_t region);
APTDEC_API void aptdec_flip    (aptdec_image_t *img, aptdec_region_t region);
APTDEC_API void aptdec_stretch (aptdec_image_t *img, aptdec_region_t region);
APTDEC_API void aptdec_equalize(aptdec_image_t *img, aptdec_region_t region);
APTDEC_API  int aptdec_crop    (aptdec_image_t *img);
APTDEC_API void aptdec_strip   (aptdec_image_t* img);

// Composite two RGB values as layers, in most cases bottom_a will be 1.0f
APTDEC_API aptdec_rgb_t aptdec_composite_rgb(aptdec_rgb_t top, float top_a, aptdec_rgb_t bottom, float bottom_a);

// Apply a gradient such as aptdec_temperature_gradient
// If gradient is less than 256 elements it is the callers responsibility
// that `val` does not exceed the length of the gradient
APTDEC_API aptdec_rgb_t aptdec_gradient(const uint32_t *gradient, uint8_t val);

static const aptdec_region_t APTDEC_REGION_CHA      = { APTDEC_CHA_OFFSET,  APTDEC_CH_WIDTH };
static const aptdec_region_t APTDEC_REGION_CHB      = { APTDEC_CHB_OFFSET,  APTDEC_CH_WIDTH };
static const aptdec_region_t APTDEC_REGION_CHA_FULL = { 0,                  APTDEC_IMG_WIDTH/2 };
static const aptdec_region_t APTDEC_REGION_CHB_FULL = { APTDEC_IMG_WIDTH/2, APTDEC_IMG_WIDTH/2 };
static const aptdec_region_t APTDEC_REGION_FULL     = { 0,                  APTDEC_IMG_WIDTH };

APTDEC_API extern const uint32_t aptdec_temperature_gradient[256];
APTDEC_API extern const uint32_t aptdec_precipitation_gradient[58];

#ifdef __cplusplus
}
#endif

#endif
