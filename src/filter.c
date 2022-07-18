/*
 * aptdec - A lightweight FOSS (NOAA) APT decoder
 * Copyright (C) 2004-2009 Thierry Leconte (F4DWV) 2019-2022 Xerbo (xerbo@protonmail.com)
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
#include "filter.h"
#include "util.h"

float convolve(const float *in, const float *taps, size_t len) {
	float sum = 0.0;
	for (size_t i = 0; i < len; i++) {
		sum += in[i] * taps[i];
	}

	return sum;
}

float complex hilbert_transform(const float *in, const float *taps, size_t len) {
	float i = 0.0;
	float q = 0.0;

	for (size_t k = 0; k < len; k++) {
		q += in[2*k] * taps[k];
		i += in[2*k];
	}

	i = in[len-1] - (i / len);
	return i + q*I;
}

float interpolating_convolve(const float *in, const float *taps, size_t len, float offset, float delta) {
	float out = 0.0;
	float n = offset;

	for (size_t i = 0; i < (len-1)/delta-1; n += delta, i++) {
		int k = (int)floor(n);
		float alpha = n - k;

		out += in[i] * (taps[k] * (1.0f-alpha) + taps[k + 1] * alpha);
	}
	return out;
}
