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

#include "filter.h"

#include <math.h>
// SSE2 intrinsics
#ifdef __x86_64__
#include <emmintrin.h>
#endif

#include "algebra.h"

// Blackman window
// https://en.wikipedia.org/wiki/Window_function#Blackman_window
static float blackman(float n, size_t ntaps) {
    n = (M_PIf * n) / (float)(ntaps - 1);
    return 0.42f - 0.5f*cosf(2 * n) + 0.08f*cosf(4 * n);
}

// Sinc low pass with blackman window.
// https://tomroelandts.com/articles/how-to-create-a-simple-low-pass-filter
void design_low_pass(float *taps, float samp_rate, float cutoff, size_t ntaps) {
    for (size_t i = 0; i < ntaps; i++) {
        int x = i - ntaps/2;
        taps[i] = sincf(2.0f * cutoff/samp_rate * (float)x);
        taps[i] *= blackman(i, ntaps);
    }

    // Achieve unity gain
    normalizef(taps, ntaps);
}

// Hilbert filter with blackman window.
// https://www.recordingblogs.com/wiki/hilbert-transform
void design_hilbert(float *taps, size_t ntaps) {
    for (size_t i = 0; i < ntaps; i++) {
        int x = i - ntaps/2;

        if (x % 2 == 0) {
            taps[i] = 0.0f;
        } else {
            taps[i] = 2.0f / (M_PIf * (float)x);
            taps[i] *= blackman(i, ntaps);
        }
    }

    // Achieve unity gain
    normalizef(taps, ntaps);
}

float convolve(const float *in, const float *taps, size_t len) {
#ifdef __SSE2__
    __m128 sum = _mm_setzero_ps();

    size_t i;
    for (i = 0; i < len - 3; i += 4) {
        __m128 _taps = _mm_loadu_ps(&taps[i]);
        __m128 _in = _mm_loadu_ps(&in[i]);
        sum = _mm_add_ps(sum, _mm_mul_ps(_taps, _in));
    }

    float residual = 0.0f;
    for (; i < len; i++) {
        residual += in[i] * taps[i];
    }

    __attribute__((aligned(16))) float _sum[4];
    _mm_store_ps(_sum, sum);
    return _sum[0] + _sum[1] + _sum[2] + _sum[3] + residual;
#else
    float sum = 0.0f;
    for (size_t i = 0; i < len; i++) {
        sum += in[i] * taps[i];
    }

    return sum;
#endif
}

complexf_t hilbert_transform(const float *in, const float *taps, size_t len) {
    return complex_build(in[len / 2], convolve(in, taps, len));
}

float interpolating_convolve(const float *in, const float *taps, size_t len, float offset) {
#ifdef _MSC_VER
    float *_taps = (float *)_alloca(len * sizeof(float));
#else
    float _taps[len];
#endif

    for (size_t i = 0; i < len; i++) {
        float next = (i == len-1) ? 0.0f : taps[i+1];
        _taps[i] = taps[i]*(1.0f-offset) + next*offset;
    }

    return convolve(in, _taps, len);
}
