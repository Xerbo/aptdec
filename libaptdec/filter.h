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

#ifndef LIBAPTDEC_FILTER_H_
#define LIBAPTDEC_FILTER_H_

#include <complex.h>
#include <stddef.h>

#ifdef _MSC_VER
typedef _Fcomplex complexf_t;
#define complex_build(real, imag) _FCbuild(real, imag)
#define complex_multiply(a, b) _FCmulcc(a, b)
#else
typedef complex float complexf_t;
#define complex_build(real, imag) ((real) + (imag)*I)
#define complex_multiply(a, b) ((a) * (b))
#endif

void design_low_pass(float *taps, float samp_rate, float cutoff, size_t ntaps);
void design_hilbert(float *taps, size_t ntaps);

float convolve(const float *in, const float *taps, size_t len);
complexf_t hilbert_transform(const float *in, const float *taps, size_t len);
float interpolating_convolve(const float *in, const float *taps, size_t len, float offset);

#endif
