/*
 * aptdec - A lightweight FOSS (NOAA) APT decoder
 * Copyright (C) 2004-2009 Thierry Leconte (F4DWV) 2019-2023 Xerbo (xerbo@protonmail.com)
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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <aptdec.h>

#include "algebra.h"
#include "util.h"
#include "calibration.h"

#define APT_COUNT_RATIO (1023.0f/255.0f)

apt_image_t apt_image_clone(apt_image_t img) {
    apt_image_t _img = img;
    _img.data = calloc(APT_IMG_WIDTH * img.rows, sizeof(uint8_t));
    memcpy(_img.data, img.data, APT_IMG_WIDTH * img.rows);
    return _img;
}

static void decode_telemetry(const float *data, size_t rows, size_t offset, float *wedges) {
    // Calculate row average
    float *telemetry_rows = calloc(rows, sizeof(float));
    for (size_t y = 0; y < rows; y++) {
        telemetry_rows[y] = meanf(&data[y*APT_IMG_WIDTH + offset + APT_CH_WIDTH], APT_TELEMETRY_WIDTH);
    }

    // Calculate relative telemetry offset (via step detection, i.e. wedge 8 to 9)
    size_t telemetry_offset = 0;
    float max_difference = 0.0f;
    for (size_t y = APT_WEDGE_HEIGHT; y <= rows - APT_WEDGE_HEIGHT; y++) {
        float difference = sumf(&telemetry_rows[y - APT_WEDGE_HEIGHT], APT_WEDGE_HEIGHT) - sumf(&telemetry_rows[y], APT_WEDGE_HEIGHT);

        // Find the maximum difference
        if (difference > max_difference) {
            max_difference = difference;
            telemetry_offset = (y + 64) % APT_FRAME_LEN;
        }
    }

    // Find the least noisy frame (via standard deviation)
    float best_noise = FLT_MAX;
    size_t best_frame = 0;
    for (size_t y = telemetry_offset; y < rows-APT_FRAME_LEN; y += APT_FRAME_LEN) {
        float noise = 0.0f;
        for (size_t i = 0; i < APT_FRAME_WEDGES; i++) {
            noise += standard_deviation(&telemetry_rows[y + i*APT_WEDGE_HEIGHT], APT_WEDGE_HEIGHT);
        }

        if (noise < best_noise) {
            best_noise = noise;
            best_frame = y;
        }
    }

    for (size_t i = 0; i < APT_FRAME_WEDGES; i++) {
        wedges[i] = meanf(&telemetry_rows[best_frame + i*APT_WEDGE_HEIGHT], APT_WEDGE_HEIGHT);
    }

    free(telemetry_rows);
}

static float average_spc(apt_image_t *img, size_t offset) {
    float *rows = calloc(img->rows, sizeof(float));
    float average = 0.0f;
    for (size_t y = 0; y < img->rows; y++) {
        float row_average = 0.0f;
        for (size_t x = 0; x < APT_SPC_WIDTH; x++) {
            row_average += img->data[y*APT_IMG_WIDTH + offset - APT_SPC_WIDTH + x];
        }
        row_average /= (float)APT_SPC_WIDTH;

        rows[y] = row_average;
        average += row_average;
    }
    average /= (float)img->rows;

    float weighted_average = 0.0f;
    size_t n = 0;
    for (size_t y = 0; y < img->rows; y++) {
        if (fabsf(rows[y] - average) < 50.0f) {
            weighted_average += rows[y];
            n++;
        }
    }

    free(rows);
    return weighted_average / (float)n;
}

apt_image_t apt_normalize(const float *data, size_t rows, apt_satellite_t satellite, int *error) {
    apt_image_t img;
    img.rows = rows;
    img.satellite = satellite;

    *error = 0;
    if (rows < APTDEC_NORMALIZE_ROWS) {
        *error = -1;
        return img;
    }

    // Decode and average wedges
    float wedges[APT_FRAME_WEDGES];
    float wedges_cha[APT_FRAME_WEDGES];
    float wedges_chb[APT_FRAME_WEDGES];
    decode_telemetry(data, rows, APT_CHA_OFFSET, wedges_cha);
    decode_telemetry(data, rows, APT_CHB_OFFSET, wedges_chb);
    for (size_t i = 0; i < APT_FRAME_WEDGES; i++) {
        wedges[i] = (wedges_cha[i] + wedges_chb[i]) / 2.0f;
    }

    // Calculate normalization
    const float reference[9] = { 31, 63, 95, 127, 159, 191, 223, 255, 0 };
    linear_t normalization = linear_regression(wedges, reference, 9);
    if (normalization.a < 0.0f) {
        *error = -1;
        return img;
    }

    // Normalize telemetry
    for (size_t i = 0; i < APT_FRAME_WEDGES; i++) {
        img.telemetry[0][i] = linear_calc(wedges_cha[i], normalization);
        img.telemetry[1][i] = linear_calc(wedges_chb[i], normalization);
    }

    // Decode channel ID wedges
    img.ch[0] = roundf(img.telemetry[0][15] / 32.0f);
    img.ch[1] = roundf(img.telemetry[1][15] / 32.0f);
    if (img.ch[0] < 1 || img.ch[0] > 6) img.ch[0] = AVHRR_CHANNEL_UNKNOWN; 
    if (img.ch[1] < 1 || img.ch[1] > 6) img.ch[1] = AVHRR_CHANNEL_UNKNOWN; 

    // Normalize and quantize image data
    img.data = (uint8_t *)malloc(rows * APT_IMG_WIDTH);
    for (size_t i = 0; i < rows * APT_IMG_WIDTH; i++) {
        float count = linear_calc(data[i], normalization);
        img.data[i] = clamp_int(roundf(count), 0, 255);
    }

    // Get space brightness
    img.space_view[0] = average_spc(&img, APT_CHA_OFFSET);
    img.space_view[1] = average_spc(&img, APT_CHB_OFFSET);

    return img;
}

static void make_thermal_lut(apt_image_t *img, avhrr_channel_t ch, int satellite, float *lut) {
    ch -= 4;
    const calibration_t calibration = get_calibration(satellite);
    const float Ns = calibration.cor[ch].Ns;
    const float Vc = calibration.rad[ch].vc;
    const float A  = calibration.rad[ch].A;
    const float B  = calibration.rad[ch].B;

    // Compute PRT temperature
    float T[4] = { 0.0f };
    for (size_t n = 0; n < 4; n++) {
        T[n] = quadratic_calc(img->telemetry[1][n + 9] * APT_COUNT_RATIO, calibration.prt[n]);
    }

    float Tbb = meanf(T, 4);      // Blackbody temperature
    float Tbbstar = A + Tbb * B;  // Effective blackbody temperature

    float Nbb = C1 * pow(Vc, 3) / (expf(C2 * Vc / Tbbstar) - 1.0f);  // Blackbody radiance

    float Cs = img->space_view[1] * APT_COUNT_RATIO;
    float Cb = img->telemetry[1][14] * APT_COUNT_RATIO;
    
    for (size_t i = 0; i < 256; i++) {
        float Nl = Ns + (Nbb - Ns) * (Cs - i * APT_COUNT_RATIO) / (Cs - Cb);      // Linear radiance estimate
        float Nc = quadratic_calc(Nl, calibration.cor[ch].quadratic);  // Non-linear correction
        float Ne = Nl + Nc;                                            // Corrected radiance

        float Testar = C2 * Vc / logf(C1 * powf(Vc, 3) / Ne + 1.0);  // Equivalent black body temperature
        float Te = (Testar - A) / B;                                 // Temperature (kelvin)

        // Convert to celsius
        lut[i] = Te - 273.15;
    }
}

int apt_calibrate_thermal(apt_image_t *img, apt_region_t region) {
    if (img->ch[1] != AVHRR_CHANNEL_4 && img->ch[1] != AVHRR_CHANNEL_5 && img->ch[1] != AVHRR_CHANNEL_3B) {
        return 1;
    }

    float lut[256] = { 0.0f };
    make_thermal_lut(img, img->ch[1], img->satellite, lut);

    for (size_t y = 0; y < img->rows; y++) {
        for (size_t x = 0; x < region.width; x++) {
            float temperature = lut[img->data[y * APT_IMG_WIDTH + region.offset + x]];
            img->data[y * APT_IMG_WIDTH + region.offset + x] = clamp_int(roundf((temperature + 100.0) / 160.0 * 255.0), 0, 255);
        }
    }

    return 0;
}

static float calibrate_pixel_visible(float value, int channel, calibration_t cal) {
    if (value > cal.visible[channel].cutoff) {
        return linear_calc(value * APT_COUNT_RATIO, cal.visible[channel].high) / 100.0f * 255.0f;
    } else {
        return linear_calc(value * APT_COUNT_RATIO, cal.visible[channel].low) / 100.0f * 255.0f;
    }
}

int apt_calibrate_visible(apt_image_t *img, apt_region_t region) {
    if (img->ch[0] != AVHRR_CHANNEL_1 && img->ch[0] != AVHRR_CHANNEL_2) {
        return 1;
    }

    calibration_t calibration = get_calibration(img->satellite);
    for (size_t y = 0; y < img->rows; y++) {
        for (size_t x = 0; x < region.width; x++) {
            float albedo = calibrate_pixel_visible(img->data[y * APT_IMG_WIDTH + region.offset + x], img->ch[0]-1, calibration);
            img->data[y * APT_IMG_WIDTH + region.offset + x] = clamp_int(roundf(albedo), 0, 255);
        }
    }

    return 0;
}
