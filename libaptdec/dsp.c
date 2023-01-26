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

const float sync_pattern[] = {-1, -1, -1, -1, 1, 1, -1, -1, 1, 1, -1, -1, 1,  1,  -1, -1, 1,  1,  -1, -1,
                              1,  1,  -1, -1, 1, 1, -1, -1, 1, 1, -1, -1, -1, -1, -1, -1, -1, -1, 0};
#define SYNC_SIZE (sizeof(sync_pattern)/sizeof(sync_pattern[0]))

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

struct aptdec {
    float sample_rate;
    float sync_frequency;

    pll_t *pll;
    fir_t *hilbert;

    float low_pass[LOW_PASS_SIZE];
    float row_buffer[APTDEC_IMG_WIDTH + SYNC_SIZE + 2];

    float interpolator_buffer[APTDEC_BUFFER_SIZE];
    size_t interpolator_n;
    float interpolator_offset;
};

char *aptdec_get_version(void) {
    return VERSION;
}

fir_t *fir_init(size_t max_size, size_t ntaps) {
    fir_t *fir = calloc(1, sizeof(fir_t));
    fir->ntaps = ntaps;
    fir->ring_size = max_size + ntaps;
    fir->taps = calloc(ntaps, sizeof(float));
    fir->ring_buffer = calloc(max_size + ntaps, sizeof(float));
    return fir;
}

void fir_free(fir_t *fir) {
    free(fir->ring_buffer);
    free(fir->taps);
    free(fir);
}

pll_t *pll_init(float alpha, float beta, float min_freq, float max_freq, float sample_rate) {
    pll_t *pll = calloc(1, sizeof(pll_t));
    pll->alpha = alpha;
    pll->beta = beta;
    pll->min_freq = M_TAUf * min_freq / sample_rate;
    pll->max_freq = M_TAUf * max_freq / sample_rate;
    pll->phase = 0.0f;
    pll->freq = 0.0f;
    return pll;
}

aptdec_t *aptdec_init(float sample_rate) {
    if (sample_rate > 96000 || sample_rate < (CARRIER_FREQ + APTDEC_IMG_WIDTH) * 2.0f) {
        return NULL;
    }

    aptdec_t *aptdec = calloc(1, sizeof(aptdec_t));
    aptdec->sample_rate = sample_rate;
    aptdec->sync_frequency = 1.0f;
    aptdec->interpolator_n = APTDEC_BUFFER_SIZE;
    aptdec->interpolator_offset = 0.0f;

    // PLL configuration
    // https://www.trondeau.com/blog/2011/8/13/control-loop-gain-values.html
    float damp = 0.7f;
    float bw = 0.005f;
    float alpha = (4.0f * damp * bw) / (1.0f + 2.0f * damp * bw + bw * bw);
    float beta = (4.0f * bw * bw) / (1.0f + 2.0f * damp * bw + bw * bw);
    aptdec->pll = pll_init(alpha, beta, CARRIER_FREQ-MAX_CARRIER_OFFSET, CARRIER_FREQ+MAX_CARRIER_OFFSET, sample_rate);
    if (aptdec->pll == NULL) {
        free(aptdec);
        return NULL;
    }

    // Hilbert transform
    aptdec->hilbert = fir_init(APTDEC_BUFFER_SIZE, 31);
    if (aptdec->hilbert == NULL) {
        free(aptdec->pll);
        free(aptdec);
        return NULL;
    }
    design_hilbert(aptdec->hilbert->taps, aptdec->hilbert->ntaps);

    design_low_pass(aptdec->low_pass, aptdec->sample_rate, (2080.0f + CARRIER_FREQ) / 2.0f, LOW_PASS_SIZE);

    return aptdec;
}

void aptdec_free(aptdec_t *aptdec) {
    fir_free(aptdec->hilbert);
    free(aptdec->pll);
    free(aptdec);
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

static int am_demod(aptdec_t *aptdec, float *out, size_t count, aptdec_callback_t callback, void *context) {
    size_t read = callback(&aptdec->hilbert->ring_buffer[aptdec->hilbert->ntaps], count, context);

    for (size_t i = 0; i < read; i++) {
        complexf_t sample = hilbert_transform(&aptdec->hilbert->ring_buffer[i], aptdec->hilbert->taps, aptdec->hilbert->ntaps);
        out[i] = crealf(pll_work(aptdec->pll, sample));
    }

    memcpy(aptdec->hilbert->ring_buffer, &aptdec->hilbert->ring_buffer[read], aptdec->hilbert->ntaps*sizeof(float));

    return read;
}

static int get_pixels(aptdec_t *aptdec, float *out, size_t count, aptdec_callback_t callback, void *context) {
    float ratio = aptdec->sample_rate / (4160.0f * aptdec->sync_frequency);

    for (size_t i = 0; i < count; i++) {
        // Get more samples if there are less than `LOW_PASS_SIZE` available
        if (aptdec->interpolator_n + LOW_PASS_SIZE > APTDEC_BUFFER_SIZE) {
            memcpy(aptdec->interpolator_buffer, &aptdec->interpolator_buffer[aptdec->interpolator_n], (APTDEC_BUFFER_SIZE-aptdec->interpolator_n) * sizeof(float));

            size_t read = am_demod(aptdec, &aptdec->interpolator_buffer[APTDEC_BUFFER_SIZE-aptdec->interpolator_n], aptdec->interpolator_n, callback, context);
            if (read != aptdec->interpolator_n) {
                return i;
            }
            aptdec->interpolator_n = 0;
        }

        out[i] = interpolating_convolve(&aptdec->interpolator_buffer[aptdec->interpolator_n], aptdec->low_pass, LOW_PASS_SIZE, aptdec->interpolator_offset);

        // Do not question the sacred code
        int shift = ceilf(ratio - aptdec->interpolator_offset);
        aptdec->interpolator_offset = shift + aptdec->interpolator_offset - ratio;
        aptdec->interpolator_n += shift;
    }

    return count;
}

// Get an entire row of pixels, aligned with sync markers
int aptdec_getrow(aptdec_t *aptdec, float *row, aptdec_callback_t callback, void *context) {
    // Wrap the circular buffer
    memcpy(aptdec->row_buffer, &aptdec->row_buffer[APTDEC_IMG_WIDTH], (SYNC_SIZE + 2) * sizeof(float));

    // Get a lines worth (APTDEC_IMG_WIDTH) of samples
    if (get_pixels(aptdec, &aptdec->row_buffer[SYNC_SIZE + 2], APTDEC_IMG_WIDTH, callback, context) != APTDEC_IMG_WIDTH) {
        return 0;
    }

    // Error detector
    float left = FLT_MIN;
    float middle = FLT_MIN;
    float right = FLT_MIN;
    size_t phase = 0;

    for (size_t i = 0; i < APTDEC_IMG_WIDTH; i++) {
        float _left   = convolve(&aptdec->row_buffer[i + 0], sync_pattern, SYNC_SIZE);
        float _middle = convolve(&aptdec->row_buffer[i + 1], sync_pattern, SYNC_SIZE);
        float _right  = convolve(&aptdec->row_buffer[i + 2], sync_pattern, SYNC_SIZE);
        if (_middle > middle) {
            left = _left;
            middle = _middle;
            right = _right;
            phase = i + 1;
        }
    }

    // Frequency
    float bias = (left / middle) - (right / middle);
    aptdec->sync_frequency = 1.0f + bias / APTDEC_IMG_WIDTH / 4.0f;

    // Phase
    memcpy(&row[APTDEC_IMG_WIDTH], &aptdec->row_buffer[phase], (APTDEC_IMG_WIDTH - phase) * sizeof(float));
    memcpy(&row[APTDEC_IMG_WIDTH - phase], aptdec->row_buffer, phase * sizeof(float));

    return 1;
}
