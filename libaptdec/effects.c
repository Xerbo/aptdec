/*
 * aptdec - A lightweight FOSS (NOAA) APT decoder
 * Copyright (C) 2019-2023 Xerbo (xerbo@protonmail.com)
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

#include <aptdec.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "algebra.h"
#include "util.h"
#include "filter.h"

void apt_equalize(apt_image_t *img, apt_region_t region) {
    // Plot histogram
    size_t histogram[256] = {0};
    for (size_t y = 0; y < img->rows; y++) {
        for (size_t x = 0; x < region.width; x++) {
            histogram[img->data[y * APT_IMG_WIDTH + x + region.offset]]++;
        }
    }

    // Calculate cumulative frequency
    size_t sum = 0, cf[256] = {0};
    for (int i = 0; i < 256; i++) {
        sum += histogram[i];
        cf[i] = sum;
    }

    // Apply histogram
    int area = img->rows * region.width;
    for (size_t y = 0; y < img->rows; y++) {
        for (size_t x = 0; x < region.width; x++) {
            int k = (int)img->data[y * APT_IMG_WIDTH + x + region.offset];
            img->data[y * APT_IMG_WIDTH + x + region.offset] = (255.0f / area) * cf[k];
        }
    }
}

// Brightness calibrate, including telemetry
static void image_apply_linear(uint8_t *data, int rows, int offset, int width, linear_t regr) {
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < width; x++) {
            float pv = linear_calc(data[y * APT_IMG_WIDTH + x + offset], regr);
            data[y * APT_IMG_WIDTH + x + offset] = clamp_int(roundf(pv), 0, 255);
        }
    }
}

void apt_stretch(apt_image_t *img, apt_region_t region) {
    // Plot histogram
    size_t histogram[256] = { 0 };
    for (size_t y = 0; y < img->rows; y++) {
        for (size_t x = 0; x < region.width; x++) {
            histogram[img->data[y*APT_IMG_WIDTH + x + region.offset]]++;
        }
    }

    // Calculate cumulative frequency
    size_t sum = 0;
    size_t cf[256] = { 0 };
    for (size_t i = 0; i < 256; i++) {
        sum += histogram[i];
        cf[i] = sum;
    }

    // Find min/max points (1st percentile)
    int min = -1, max = -1;
    for (size_t i = 0; i < 256; i++) {
        if ((float)cf[i] / (float)sum < 0.01f && min == -1) {
            min = i;
        }
        if ((float)cf[i] / (float)sum > 0.99f && max == -1) {
            max = i;
            break;
        }
    }

    float a = 255.0f / (max - min);
    float b = a * -min;
    image_apply_linear(img->data, img->rows, region.offset, region.width, (linear_t){a, b});
}



// Median denoise (with deviation threshold)
void apt_denoise(apt_image_t *img, apt_region_t region) {
    for (size_t y = 1; y < img->rows - 1; y++) {
        for (size_t x = 1; x < region.width - 1; x++) {
            float pixels[9];
            int pixeln = 0;
            for (int y2 = -1; y2 < 2; y2++) {
                for (int x2 = -1; x2 < 2; x2++) {
                    pixels[pixeln++] = img->data[(y + y2) * APT_IMG_WIDTH + (x + region.offset) + x2];
                }
            }

            if (standard_deviation(pixels, 9) > 15) {
                img->data[y * APT_IMG_WIDTH + x + region.offset] = medianf(pixels, 9);
            }
        }
    }
}

// Flips a channel, for northbound passes
void apt_flip(apt_image_t *img, apt_region_t region) {
    for (size_t y = 1; y < img->rows; y++) {
        for (int x = 1; x < ceil(region.width / 2.0f); x++) {
            // Flip top-left & bottom-right
            swap_uint8(
                &img->data[(img->rows - y) * APT_IMG_WIDTH + region.offset + x],
                &img->data[y * APT_IMG_WIDTH + region.offset + (region.width - x)]
            );
        }
    }
}

// Calculate crop to remove noise from the start and end of an image
#define NOISE_THRESH 2600.0

#include "filter.h"

int apt_crop(apt_image_t *img) {
    const float sync_pattern[] = {-1, -1, -1, -1, 1, 1, -1, -1, 1, 1, -1, -1, 1,  1,  -1, -1, 1,  1,  -1, -1,
                                  1,  1,  -1, -1, 1, 1, -1, -1, 1, 1, -1, -1, -1, -1, -1, -1, -1, -1, 0};

    float spc_rows[img->rows];
    int startCrop = 0;
    int endCrop = img->rows;

    for (size_t y = 0; y < img->rows; y++) {
        float temp[39];
        for (size_t i = 0; i < 39; i++) {
            temp[i] = img->data[y * APT_IMG_WIDTH + i];
        }

        spc_rows[y] = convolve(temp, &sync_pattern[0], 39);
    }

    // Find ends
    for (size_t y = 0; y < img->rows - 1; y++) {
        if (spc_rows[y] > NOISE_THRESH) {
            endCrop = y;
        }
    }
    for (size_t y = img->rows; y > 0; y--) {
        if (spc_rows[y] > NOISE_THRESH) {
            startCrop = y;
        }
    }

    printf("Crop rows: %i -> %i\n", startCrop, endCrop);

    // Ignore the noisy rows at the end
    img->rows = (endCrop - startCrop);

    // Remove the noisy rows at start
    memmove(img->data, &img->data[startCrop * APT_IMG_WIDTH], img->rows * APT_IMG_WIDTH * sizeof(float));

    return startCrop;
}
