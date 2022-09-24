/*
 * aptdec - A lightweight FOSS (NOAA) APT decoder
 * Copyright (C) 2019-2022 Xerbo (xerbo@protonmail.com)
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

#include <stddef.h>
#include <complex.h>

#ifdef _MSC_VER
typedef _Fcomplex complexf_t;
#else
typedef complex float complexf_t;
#endif

float      convolve(const float *in, const float *taps, size_t len);
complexf_t hilbert_transform(const float *in, const float *taps, size_t len);
float      interpolating_convolve(const float *in, const float *taps, size_t len, float offset, float delta);
