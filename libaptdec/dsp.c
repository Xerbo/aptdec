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

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <aptdec.h>

#include "filter.h"
#include "util.h"
#include "algebra.h"

#define LOW_PASS_SIZE 101

#define CARRIER_FREQ 2400.0f
#define MAX_CARRIER_OFFSET 10.0f

typedef struct {
    float alpha;
    float beta;
    float min_freq;
    float max_freq;

    float freq;
    float phase;
} pll_t;

typedef struct {
    float *ring_buffer;
    size_t ring_size;

    float *taps;
    size_t ntaps;
} fir_t;

struct aptdec_t {
    float sample_rate;
    float sync_frequency;

    pll_t *pll;
    fir_t *hilbert;

    float low_pass[LOW_PASS_SIZE];
};

char *aptdec_get_version(void) {
    return VERSION;
}

fir_t *fir_init(size_t max_size, size_t ntaps) {
    fir_t *fir = (fir_t *)malloc(sizeof(fir_t));
    fir->ntaps = ntaps;
    fir->ring_size = max_size + ntaps;
    fir->taps = (float *)malloc(ntaps * sizeof(float));
    fir->ring_buffer = (float *)malloc((max_size + ntaps) * sizeof(float));
    return fir;
}

void fir_free(fir_t *fir) {
    free(fir->ring_buffer);
    free(fir->taps);
    free(fir);
}

pll_t *pll_init(float alpha, float beta, float min_freq, float max_freq, float sample_rate) {
    pll_t *pll = (pll_t *)malloc(sizeof(pll_t));
    pll->alpha = alpha;
    pll->beta = beta;
    pll->min_freq = M_TAUf * min_freq / sample_rate;
    pll->max_freq = M_TAUf * max_freq / sample_rate;
    pll->phase = 0.0f;
    pll->freq = 0.0f;
    return pll;
}

aptdec_t *aptdec_init(float sample_rate) {
    if (sample_rate > 96000 || sample_rate < (CARRIER_FREQ + APT_IMG_WIDTH) * 2.0f) {
        return NULL;
    }

    aptdec_t *apt = (aptdec_t *)malloc(sizeof(aptdec_t));
    apt->sample_rate = sample_rate;
    apt->sync_frequency = 1.0f;

    // PLL configuration
    // https://www.trondeau.com/blog/2011/8/13/control-loop-gain-values.html
    float damp = 0.7f;
    float bw = 0.005f;
    float alpha = (4.0f * damp * bw) / (1.0f + 2.0f * damp * bw + bw * bw);
    float beta = (4.0f * bw * bw) / (1.0f + 2.0f * damp * bw + bw * bw);
    apt->pll = pll_init(alpha, beta, CARRIER_FREQ-MAX_CARRIER_OFFSET, CARRIER_FREQ+MAX_CARRIER_OFFSET, sample_rate);
    if (apt->pll == NULL) {
        free(apt);
        return NULL;
    }

    // Hilbert transform
    apt->hilbert = fir_init(APTDEC_BUFFER_SIZE, 31);
    if (apt->hilbert == NULL) {
        free(apt->pll);
        free(apt);
        return NULL;
    }
    design_hilbert(apt->hilbert->taps, apt->hilbert->ntaps);

    design_low_pass(apt->low_pass, apt->sample_rate, (2080.0f + CARRIER_FREQ) / 2.0f, LOW_PASS_SIZE);

    return apt;
}

void aptdec_free(aptdec_t *apt) {
    fir_free(apt->hilbert);
    free(apt->pll);
    free(apt);
}

static complexf_t pll_work(pll_t *pll, complexf_t in) {
    // Internal oscillator (90deg offset)
    complexf_t osc = complex_build(cosf(pll->phase), -sinf(pll->phase));
    in = complex_multiply(in, osc);

    // Error detector
    float error = cargf(in);

    // Loop filter (single pole IIR)
    pll->freq += pll->beta * error;
    pll->freq = clamp(pll->freq, pll->min_freq, pll->max_freq);
    pll->phase += pll->freq + (pll->alpha * error);
    pll->phase = remainderf(pll->phase, M_TAUf);

    return in;
}

static int am_demod(aptdec_t *apt, float *out, size_t count, aptdec_callback_t callback, void *context) {
    size_t read = callback(&apt->hilbert->ring_buffer[apt->hilbert->ntaps], count, context);

    for (size_t i = 0; i < read; i++) {
        complexf_t sample = hilbert_transform(&apt->hilbert->ring_buffer[i], apt->hilbert->taps, apt->hilbert->ntaps);
        out[i] = crealf(pll_work(apt->pll, sample));
    }

    memcpy(apt->hilbert->ring_buffer, &apt->hilbert->ring_buffer[read], apt->hilbert->ntaps*sizeof(float));

    return read;
}

static int get_pixels(aptdec_t *apt, float *out, size_t count, aptdec_callback_t callback, void *context) {
    static float buffer[APTDEC_BUFFER_SIZE];
    static size_t n = APTDEC_BUFFER_SIZE;
    static float offset = 0.0;

    float ratio = apt->sample_rate / (4160.0f * apt->sync_frequency);

    for (size_t i = 0; i < count; i++) {
        // Get more samples if there are less than `LOW_PASS_SIZE` available
        if (n + LOW_PASS_SIZE > APTDEC_BUFFER_SIZE) {
            memcpy(buffer, &buffer[n], (APTDEC_BUFFER_SIZE-n) * sizeof(float));

            size_t read = am_demod(apt, &buffer[APTDEC_BUFFER_SIZE-n], n, callback, context);
            if (read != n) {
                return i;
            }
            n = 0;
        }

        out[i] = interpolating_convolve(&buffer[n], apt->low_pass, LOW_PASS_SIZE, offset);

        // Do not question the sacred code
        int shift = ceilf(ratio - offset);
        offset = shift + offset - ratio;
        n += shift;
    }

    return count;
}

const float sync_pattern[] = {-1, -1, -1, -1, 1, 1, -1, -1, 1, 1, -1, -1, 1,  1,  -1, -1, 1,  1,  -1, -1,
                              1,  1,  -1, -1, 1, 1, -1, -1, 1, 1, -1, -1, -1, -1, -1, -1, -1, -1, 0};
#define SYNC_SIZE (sizeof(sync_pattern)/sizeof(sync_pattern[0]))

// Get an entire row of pixels, aligned with sync markers
int aptdec_getrow(aptdec_t *apt, float *row, aptdec_callback_t callback, void *context) {
    static float pixels[APT_IMG_WIDTH + SYNC_SIZE + 2];

    // Wrap the circular buffer
    memcpy(pixels, &pixels[APT_IMG_WIDTH], (SYNC_SIZE + 2) * sizeof(float));
    // Get a lines worth (APT_IMG_WIDTH) of samples
    if (get_pixels(apt, &pixels[SYNC_SIZE + 2], APT_IMG_WIDTH, callback, context) != APT_IMG_WIDTH) {
        return 0;
    }

    // Error detector
    float left = FLT_MIN;
    float middle = FLT_MIN;
    float right = FLT_MIN;
    size_t phase = 0;

    for (size_t i = 0; i < APT_IMG_WIDTH; i++) {
        float _left   = convolve(&pixels[i + 0], sync_pattern, SYNC_SIZE);
        float _middle = convolve(&pixels[i + 1], sync_pattern, SYNC_SIZE);
        float _right  = convolve(&pixels[i + 2], sync_pattern, SYNC_SIZE);
        if (_middle > middle) {
            left = _left;
            middle = _middle;
            right = _right;
            phase = i + 1;
        }
    }

    // Frequency
    float bias = (left / middle) - (right / middle);
    apt->sync_frequency = 1.0f + bias / APT_IMG_WIDTH / 2.0f;

    // Phase
    memcpy(&row[APT_IMG_WIDTH], &pixels[phase], (APT_IMG_WIDTH - phase) * sizeof(float));
    memcpy(&row[APT_IMG_WIDTH - phase], pixels, phase * sizeof(float));

    return 1;
}
