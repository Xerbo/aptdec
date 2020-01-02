/* 
 *  This file is part of Aptdec.
 *  Copyright (c) 2004-2009 Thierry Leconte (F4DWV), Xerbo (xerbo@protonmail.com) 2019-2020
 *
 *  Aptdec is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "filter.h"
#include "filtercoeff.h"

// In case your C compiler is so old that Pi hadn't been invented yet
#ifndef M_PI
#define M_PI 3.141592653589793
#endif

// Block sizes
#define BLKAMP 1024
#define BLKIN 1024

#define Fc 2400.0
#define DFc 50.0
#define PixelLine 2080
#define Fp (2 * PixelLine)
#define RSMULT 15
#define Fi (Fp * RSMULT)

extern int getsample(float *inbuff, int count);

static double Fe;

static double offset = 0.0;
static double FreqLine = 1.0;

static double FreqOsc;
static double K1, K2;

// Check the input sample rate and calculate some constants
int init_dsp(double F) {
	if(F > Fi) return(1);
	if(F < Fp) return(-1);
	Fe = F;

	K1 = DFc / Fe;
	K2 = K1 * K1 / 2.0;
	// Number of samples per cycle
	FreqOsc = Fc / Fe;

	return(0);
}

/* Fast phase estimator
 * Calculates the phase angle of a signal from a IQ sample
 */
static inline double Phase(double I, double Q) {
	double angle, r;
	int s;

	if(I == 0.0 && Q == 0.0)
		return(0.0);

   	if (Q < 0) {
		s = -1;
		Q = -Q;
   	} else {
		s = 1;
   	}
	
   	if (I >= 0) {
    	r = (I - Q) / (I + Q);
    	angle = 0.25 - 0.25 * r;
   	} else {
    	r = (I + Q) / (Q - I);
    	angle = 0.75 - 0.25 * r;
   	}

  	if(s > 0)
  		return(angle);
  	else
  		return(-angle);
}

/* Phase locked loop
 * https://en.wikipedia.org/wiki/Phase-locked_loop
 * https://arachnoid.com/phase_locked_loop/
 * https://simple.wikipedia.org/wiki/Phase-locked_loop
 */
static double pll(double I, double Q) {
	// PLL coefficient
    static double PhaseOsc = 0.0;
    double Io, Qo;
    double Ip, Qp;
    double DPhi;

	// Quadrature oscillator / reference
    Io = cos(PhaseOsc);
    Qo = sin(PhaseOsc);

	// Phase detector
    Ip = I * Io + Q * Qo;
    Qp = Q * Io - I * Qo;
    DPhi = Phase(Ip, Qp);

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

    return(Ip);
}

// Convert samples into pixels
static int getamp(double *ampbuff, int count) {
    static float inbuff[BLKIN];
    static int idxin = 0;
    static int nin = 0;

    for (int n = 0; n < count; n++) {
    	double I, Q;

		// Get some more samples when needed
		if (nin < IQFilterLen * 2 + 2) {
			// Number of samples read
			int res;
			memmove(inbuff, &(inbuff[idxin]), nin * sizeof(float));
			idxin = 0;
			
			// Read some samples
			res = getsample(&(inbuff[nin]), BLKIN - nin);
			nin += res;
			
			// Make sure there is enough samples to continue
			if (nin < IQFilterLen * 2 + 2)
				return(n);
		}

		// Process read samples into a brightness value
		iqfir(&inbuff[idxin], iqfilter, IQFilterLen, &I, &Q);
		ampbuff[n] = pll(I, Q);

		// Increment current sample
		idxin++;
		nin--;
    }

    return(count);
}

// Sub-pixel offsetting + FIR compensation
int getpixelv(float *pvbuff, int count) {
	// Amplitude buffer
    static double ampbuff[BLKAMP];
    static int nam = 0;
    static int idxam = 0;

    double mult;

	// Sub-pixel offset
    mult = (double) Fi / Fe * FreqLine;
    int m = RSFilterLen / mult + 1;

    for (int n = 0; n < count; n++) {
		int shift;

		if (nam < m) {
			int res;
			memmove(ampbuff, &(ampbuff[idxam]), nam * sizeof(double));
			idxam = 0;
			res = getamp(&(ampbuff[nam]), BLKAMP - nam);
			nam += res;
			if (nam < m)
				return(n);
		}

		// Gaussian FIR compensation filter
		pvbuff[n] = rsfir(&(ampbuff[idxam]), rsfilter, RSFilterLen, offset, mult) * mult * 256.0;

		shift = ((int) floor((RSMULT - offset) / mult)) + 1;
		offset = shift * mult + offset - RSMULT;

		idxam += shift;
		nam -= shift;
    }
    return(count);
}

// Get an entire row of pixels, aligned with sync markers
double minDoppler = 100;
int getpixelrow(float *pixelv, int nrow, int *zenith) {
    static float pixels[PixelLine + SyncFilterLen];
    static int npv;
    static int synced = 0;
    static double max = 0.0;

    double corr, ecorr, lcorr;
    int res;

	// Move the row buffer into the the image buffer
    if (npv > 0)
		memmove(pixelv, pixels, npv * sizeof(float));

	// Get the sync line
    if (npv < SyncFilterLen + 2) {
		res = getpixelv(&(pixelv[npv]), SyncFilterLen + 2 - npv);
		npv += res;
		// Exit if there are no pixels left
		if (npv < SyncFilterLen + 2) return(0);
    }

    // Calculate the frequency offset
    ecorr = fir(pixelv, Sync, SyncFilterLen);
    corr = fir(&(pixelv[1]), Sync, SyncFilterLen - 1);
    lcorr = fir(&(pixelv[2]), Sync, SyncFilterLen - 2);
    FreqLine = 1.0+((ecorr-lcorr) / corr / PixelLine / 4.0);

	// Find the point in which ecorr and lcorr intercept, make sure that it's not too perfect
	if(fabs(lcorr - ecorr) < minDoppler && fabs(lcorr - ecorr) > 1){
		minDoppler = fabs(lcorr - ecorr);
		*zenith = nrow;
	}
	
	// The point in which the pixel offset is recalculated
    if (corr < 0.75 * max) {
		synced = 0;
		FreqLine = 1.0;
    }
    max = corr;

	if (synced < 8) {
		int mshift;

		if (npv < PixelLine + SyncFilterLen) {
			res = getpixelv(&(pixelv[npv]), PixelLine + SyncFilterLen - npv);
			npv += res;
			if (npv < PixelLine + SyncFilterLen)
				return(0);
		}

		// Test every possible position until we get the best result
		mshift = 0;
		for (int shift = 0; shift < PixelLine; shift++) {
			double corr;

			corr = fir(&(pixelv[shift + 1]), Sync, SyncFilterLen);
			if (corr > max) {
				mshift = shift;
				max = corr;
			}
		}

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
		res = getpixelv(&(pixelv[npv]), PixelLine - npv);
		npv += res;
		if (npv < PixelLine)
			return(0);
    }

	// Move the sync lines into the output buffer with the calculated offset
    if (npv == PixelLine) {
		npv = 0;
    } else {
		memmove(pixels, &(pixelv[PixelLine]), (npv - PixelLine) * sizeof(float));
		npv -= PixelLine;
    }

    return(1);
}