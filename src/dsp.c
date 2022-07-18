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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "apt.h"
#include "filter.h"
#include "taps.h"

// In case your C compiler is so old that Pi hadn't been invented yet
#ifndef M_PI
#define M_PI 3.141592653589793
#endif

// Block sizes
#define BLKAMP 32768
#define BLKIN 32768

#define Fc 2400.0
#define DFc 50.0
#define PixelLine 2080
#define Fp (2 * PixelLine)
#define RSMULT 15
#define Fi (Fp * RSMULT)

static float Fe;

static float offset = 0.0;
static float FreqLine = 1.0;

static float FreqOsc;
static float K1, K2;

// Check the sample rate and calculate some constants
int apt_init(double F) {
	if(F > Fi) return 1;
	if(F < Fp) return -1;
	Fe = F;

	K1 = DFc / Fe;
	K2 = K1 * K1 / 2.0;
	// Number of samples per cycle
	FreqOsc = Fc / Fe;

	return 0;
}

/* Phase locked loop
 * https://arachnoid.com/phase_locked_loop/
 * Model of this filter here https://www.desmos.com/calculator/m0uadgkoee
 */
static float pll(float I2, float Q) {
	// PLL coefficient
	static float PhaseOsc = 0.0;
	float Io, Qo;
	float Ip, Qp;
	float DPhi;

	// Quadrature oscillator / reference
	Io = cos(PhaseOsc);
	Qo = sin(PhaseOsc);

	// Phase detector
	Ip = I2 * Io + Q * Qo;
	Qp = Q * Io - I2 * Qo;
	DPhi = atan2f(Qp, Ip);

	// Loop filter
	PhaseOsc += 2.0 * M_PI * (K1 * DPhi + FreqOsc);
	if (PhaseOsc > M_PI)
		PhaseOsc -= 2.0 * M_PI;
	if (PhaseOsc <= -M_PI)
		PhaseOsc += 2.0 * M_PI;

	FreqOsc += K2 * DPhi;
	if (FreqOsc > ((Fc + DFc) / Fe))
		FreqOsc = (Fc + DFc) / Fe;
	if (FreqOsc < ((Fc - DFc) / Fe))
		FreqOsc = (Fc - DFc) / Fe;

	return Ip;
}

// Convert samples into pixels
static int getamp(float *ampbuff, int count, apt_getsamples_t getsamples, void *context) {
	static float inbuff[BLKIN];
	static int idxin = 0;
	static int nin = 0;

	for (int n = 0; n < count; n++) {
		float I2, Q;

		// Get some more samples when needed
		if (nin < HILBERT_FILTER_SIZE * 2 + 2) {
			// Number of samples read
			int res;
			memmove(inbuff, &(inbuff[idxin]), nin * sizeof(float));
			idxin = 0;

			// Read some samples
			res = getsamples(context, &(inbuff[nin]), BLKIN - nin);
			nin += res;

			// Make sure there is enough samples to continue
			if (nin < HILBERT_FILTER_SIZE * 2 + 2)
				return n;
		}

		// Process read samples into a brightness value
		float complex tmp = hilbert_transform(&inbuff[idxin], hilbert_filter, HILBERT_FILTER_SIZE);
		I2 = crealf(tmp);
		Q = cimagf(tmp);
		ampbuff[n] = pll(I2, Q);

		// Increment current sample
		idxin++;
		nin--;
	}

	return count;
}

// Sub-pixel offsetting 
int getpixelv(float *pvbuff, int count, apt_getsamples_t getsamples, void *context) {
	// Amplitude buffer
	static float ampbuff[BLKAMP];
	static int nam = 0;
	static int idxam = 0;

	float mult;

	// Gaussian resampling factor
	mult = (float) Fi / Fe * FreqLine;
	int m = (int)(LOW_PASS_SIZE / mult + 1);

	for (int n = 0; n < count; n++) {
		int shift;

		if (nam < m) {
			int res;
			memmove(ampbuff, &(ampbuff[idxam]), nam * sizeof(float));
			idxam = 0;
			res = getamp(&(ampbuff[nam]), BLKAMP - nam, getsamples, context);
			nam += res;
			if (nam < m)
				return n;
		}

		pvbuff[n] = interpolating_convolve(&(ampbuff[idxam]), low_pass, LOW_PASS_SIZE, offset, mult) * mult * 256.0;

		shift = ((int) floor((RSMULT - offset) / mult)) + 1;
		offset = shift * mult + offset - RSMULT;

		idxam += shift;
		nam -= shift;
	}

	return count;
}

// Get an entire row of pixels, aligned with sync markers
int apt_getpixelrow(float *pixelv, int nrow, int *zenith, int reset, apt_getsamples_t getsamples, void *context) {
	static float pixels[PixelLine + SYNC_PATTERN_SIZE];
	static size_t npv;
	static int synced = 0;
	static float max = 0.0;
	static float minDoppler = 1000000000, previous = 0;

	if(reset) synced = 0;

	float corr, ecorr, lcorr;
	int res;

	// Move the row buffer into the the image buffer
	if (npv > 0)
		memmove(pixelv, pixels, npv * sizeof(float));

	// Get the sync line
	if (npv < SYNC_PATTERN_SIZE + 2) {
		res = getpixelv(&(pixelv[npv]), SYNC_PATTERN_SIZE + 2 - npv, getsamples, context);
		npv += res;
		if (npv < SYNC_PATTERN_SIZE + 2)
			return 0;
	}

	// Calculate the frequency offset
	ecorr = convolve(pixelv, sync_pattern, SYNC_PATTERN_SIZE);
	corr = convolve(&pixelv[1], sync_pattern, SYNC_PATTERN_SIZE - 1);
	lcorr = convolve(&pixelv[2], sync_pattern, SYNC_PATTERN_SIZE - 2);
	FreqLine = 1.0+((ecorr-lcorr) / corr / PixelLine / 4.0);

	float val = fabs(lcorr - ecorr)*0.25 + previous*0.75;
	if(val < minDoppler && nrow > 10){
		minDoppler = val;
		*zenith = nrow;
	}
	previous = fabs(lcorr - ecorr);

	// The point in which the pixel offset is recalculated
	if (corr < 0.75 * max) {
		synced = 0;
		FreqLine = 1.0;
	}
	max = corr;

	if (synced < 8) {
		int mshift;
        static int lastmshift;

		if (npv < PixelLine + SYNC_PATTERN_SIZE) {
			res = getpixelv(&(pixelv[npv]), PixelLine + SYNC_PATTERN_SIZE - npv, getsamples, context);
			npv += res;
			if (npv < PixelLine + SYNC_PATTERN_SIZE)
				return 0;
		}

		// Test every possible position until we get the best result
		mshift = 0;
		for (int shift = 0; shift < PixelLine; shift++) {
			float corr;

			corr = convolve(&(pixelv[shift + 1]), sync_pattern, SYNC_PATTERN_SIZE);
			if (corr > max) {
				mshift = shift;
				max = corr;
			}
		}

		// Stop rows dissapearing into the void
		int mshiftOrig = mshift;
		if(abs(lastmshift-mshift) > 3 && nrow != 0){
			mshift = 0;
		}
		lastmshift = mshiftOrig;

		// If we are already as aligned as we can get, just continue
		if (mshift == 0) {
			synced++;
		} else {
			memmove(pixelv, &(pixelv[mshift]), (npv - mshift) * sizeof(float));
			npv -= mshift;
			synced = 0;
			FreqLine = 1.0;
		}
	}

	// Get the rest of this row
	if (npv < PixelLine) {
		res = getpixelv(&(pixelv[npv]), PixelLine - npv, getsamples, context);
		npv += res;
		if (npv < PixelLine)
			return 0;
	}

	// Move the sync lines into the output buffer with the calculated offset
	if (npv == PixelLine) {
		npv = 0;
	} else {
		memmove(pixels, &(pixelv[PixelLine]), (npv - PixelLine) * sizeof(float));
		npv -= PixelLine;
	}

	return 1;
}
