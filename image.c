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

#include <stdio.h>
#include <string.h>
#include <sndfile.h>
#include <math.h>

#include "offsets.h"
#include "messages.h"

#define REGORDER 3
typedef struct {
	double cf[REGORDER + 1];
} rgparam;

extern void polyreg(const int m, const int n, const double x[], const double y[], double c[]);

// Compute regression
static void rgcomp(double x[16], rgparam * rgpr) {
	//                  { 0.106, 0.215, 0.324, 0.433, 0.542,  0.652, 0.78,   0.87,  0.0 }
    const double y[9] = { 31.07, 63.02, 94.96, 126.9, 158.86, 191.1, 228.62, 255.0, 0.0 };

    polyreg(REGORDER, 9, x, y, rgpr -> cf);
}

// Convert a value to 0-255 based off the provided regression curve
static double rgcal(float x, rgparam *rgpr) {
    double y, p;
	int i;
	
    for (i = 0, y = 0.0, p = 1.0; i < REGORDER + 1; i++) {
		y += rgpr->cf[i] * p;
		p = p * x;
    }
    return(y);
}

static double tele[16];
static double Cs;
static int nbtele;

void histogramEqualise(float **prow, int nrow, int offset, int width){
	// Plot histogram
	int histogram[256] = { 0 };
	for(int y = 0; y < nrow; y++)
		for(int x = 0; x < width; x++)
			histogram[(int)floor(prow[y][x+offset])]++;

	// Find min/max points
	int min = -1, max = -1;
	for(int i = 5; i < 250; i++){
		if(histogram[i]/width/(nrow/255.0) > 1.0){
			if(min == -1) min = i;
			max = i;
		}
	}

	//printf("Min Value: %i, Max Value %i\n", min, max);

	// Spread values to avoid overshoot
	min -= 5; max += 5;

	// Stretch the brightness into the new range
	for(int y = 0; y < nrow; y++)
		for(int x = 0; x < width; x++)
			prow[y][x+offset] = (prow[y][x+offset]-min) / (max-min) * 255;
}

// Brightness calibrate, including telemetry
void calibrateBrightness(float **prow, int nrow, int offset, int width, int telestart, rgparam regr[30]){
	offset -= SYNC_WIDTH+SPC_WIDTH;

	for (int n = 0; n < nrow; n++) {
		float *pixelv = prow[n];

		for (int i = 0; i < width+SYNC_WIDTH+SPC_WIDTH+TELE_WIDTH; i++) {
			float pv = pixelv[i + offset];

			// Blend between the calculated regression curves
			/* TODO: this can actually make the image look *worse*
			 * if the signal has a constant input gain.
			 */
			int k, kof;
			k = (n - telestart) / FRAME_LEN;
			if (k >= nbtele)
				k = nbtele - 1;
			kof = (n - telestart) % FRAME_LEN;

			if (kof < 64) {
				if (k < 1) {
					pv = rgcal(pv, &(regr[k]));
				} else {
					pv = rgcal(pv, &(regr[k])) * (64 + kof) / FRAME_LEN +
						 rgcal(pv, &(regr[k - 1])) * (64 - kof) / FRAME_LEN;
				}
			} else {
				if ((k + 1) >= nbtele) {
					pv = rgcal(pv, &(regr[k]));
				} else {
					pv = rgcal(pv, &(regr[k])) * (192 - kof) / FRAME_LEN +
						 rgcal(pv, &(regr[k + 1])) * (kof - 64) / FRAME_LEN;
				}
			}

			pv = CLIP(pv, 0, 255);
			pixelv[i + offset] = pv;
		}
	}
}

// Get telemetry data for thermal calibration/equalization
int calibrate(float **prow, int nrow, int offset, int width) {
    double teleline[3000] = { 0.0 };
    double wedge[16];
    rgparam regr[30];
    int n, k;
    int mtelestart = 0, telestart;
    int channel = -1;

	// Calculate average of a row of telemetry
    for (n = 0; n < nrow; n++) {
		float *pixelv = prow[n];

		// Average the center 40px
		for (int i = 3; i < 43; i++) teleline[n] += pixelv[i + offset + width];
		teleline[n] /= 40.0;
    }

	// The minimum rows required to decode a full frame
    if (nrow < 192) {
		fprintf(stderr, ERR_TELE_ROW);
		return(0);
    }

	/* Wedge 7 is white and 8 is black, which will have the largest
	 * difference in brightness, this will always be in the center of
	 * the frame and can thus be used to find the start of the frame
	 */
	double max = 0.0;
    for (n = nrow / 3 - 64; n < 2 * nrow / 3 - 64; n++) {
		float df;

		// (sum 4px below) / (sum 4px above)
		df = (teleline[n - 4] + teleline[n - 3] + teleline[n - 2] + teleline[n - 1]) /
			 (teleline[n + 0] + teleline[n + 1] + teleline[n + 2] + teleline[n + 3]);

		// Find the maximum difference
		if (df > max) {
			mtelestart = n;
			max = df;
		}
    }

	// Find the start of the first frame
    telestart = (mtelestart - FRAME_LEN/2) % FRAME_LEN;

	// Make sure that theres at least one full frame in the image
    if (nrow < telestart + FRAME_LEN) {
		fprintf(stderr, ERR_TELE_ROW);
		return(0);
    }

	// For each frame
    for (n = telestart, k = 0; n < nrow - FRAME_LEN; n += FRAME_LEN, k++) {
		float *pixelv = prow[n];
		int j;

		// Turn each wedge into a value
		for (j = 0; j < 16; j++) {
			// Average the middle 6px
			wedge[j] = 0.0;
			for (int i = 1; i < 7; i++) wedge[j] += teleline[(j * 8) + n + i];
			wedge[j] /= 6;
		}

		// Compute regression on the wedges
		rgcomp(wedge, &(regr[k]));

		// Read the telemetry values from the middle of the image
		if (k == nrow / (2*FRAME_LEN)) {
			int l;

			// Equalise
			for (j = 0; j < 16; j++) tele[j] = rgcal(wedge[j], &(regr[k]));

			/* Compare the channel ID wedge to the reference
			 * wedges, the wedge with the closest match will
			 * be the channel ID
			 */
			float min = 10000;
			for (j = 0; j < 6; j++) {
				float df;

				df = tele[15] - tele[j];
				df *= df;
				if (df < min) {
					channel = j;
					min = df;
				}
			}

			// Cs computation, still have no idea what this does
			int i;
			for (Cs = 0.0, i = 0, j = n; j < n + FRAME_LEN; j++) {
				double csline;

				for (csline = 0.0, l = 3; l < 43; l++)
					csline += pixelv[l + offset - SPC_WIDTH];
				
				csline /= 40.0;
				if (csline > 50.0) {
					Cs += csline;
					i++;
				}
			}
			Cs /= i;
			Cs = rgcal(Cs, &(regr[k]));
		}
    }
    nbtele = k;

	calibrateBrightness(prow, nrow, offset, width, telestart, regr);

    return(channel + 1);
}

// --- Temperature Calibration --- //
extern int satnum;
#include "satcal.h"

typedef struct {
    double Nbb;
    double Cs;
    double Cb;
    int ch;
} tempparam;

// IR channel temperature compensation
static void tempcomp(double t[16], int ch, tempparam *tpr) {
    double Tbb, T[4];
    double C;

    tpr -> ch = ch - 4;

    // Compute equivalent T blackbody temperature
    for (int n = 0; n < 4; n++) {
		float d0, d1, d2;

		C = t[9 + n] * 4.0;
		d0 = satcal[satnum].d[n][0];
		d1 = satcal[satnum].d[n][1];
		d2 = satcal[satnum].d[n][2];
		T[n] = d0;
		T[n] += d1 * C;
		C = C * C;
		T[n] += d2 * C;
    }
    Tbb = (T[0] + T[1] + T[2] + T[3]) / 4.0;
    Tbb = satcal[satnum].rad[tpr->ch].A + satcal[satnum].rad[tpr->ch].B * Tbb;

    // Compute radiance blackbody
    C = satcal[satnum].rad[tpr->ch].vc;
    tpr->Nbb = c1 * C * C * C / (expm1(c2 * C / Tbb));

    // Store count blackbody and space
    tpr->Cs = Cs * 4.0;
    tpr->Cb = t[14] * 4.0;
}

// IR channel temperature calibration
static double tempcal(float Ce, tempparam * rgpr) {
    double Nl, Nc, Ns, Ne;
    double T, vc;

    Ns = satcal[satnum].cor[rgpr->ch].Ns;
    Nl = Ns + (rgpr->Nbb - Ns) * (rgpr->Cs - Ce * 4.0) / (rgpr->Cs - rgpr->Cb);
    Nc = satcal[satnum].cor[rgpr->ch].b[0] +
		 satcal[satnum].cor[rgpr->ch].b[1] * Nl +
		 satcal[satnum].cor[rgpr->ch].b[2] * Nl * Nl;

    Ne = Nl + Nc;

    vc = satcal[satnum].rad[rgpr->ch].vc;
    T = c2 * vc / log1p(c1 * vc * vc * vc / Ne);
    T = (T - satcal[satnum].rad[rgpr->ch].A) / satcal[satnum].rad[rgpr->ch].B;

    // Rescale to 0-255 for -60'C to +40'C
    T = (T - 273.15 + 60.0) / 100.0 * 256.0;

    return(T);
}

// Temperature calibration wrapper
void temperature(float **prow, int nrow, int ch, int offset){
    tempparam temp;

    printf("Temperature... ");
    fflush(stdout);

    tempcomp(tele, ch, &temp);

    for (int n = 0; n < nrow; n++) {
		float *pixelv = prow[n];

		for (int i = 0; i < CH_WIDTH; i++) {
			float pv = tempcal(pixelv[i + offset], &temp);

			pv = CLIP(pv, 0, 255);
			pixelv[i + offset] = pv;
		}
    }
    printf("Done\n");
}
