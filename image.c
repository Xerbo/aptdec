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
#include <stdlib.h>

#include "common.h"
#include "offsets.h"

#define REGORDER 3
typedef struct {
	double cf[REGORDER + 1];
} rgparam_t;

extern void polyreg(const int m, const int n, const double x[], const double y[], double c[]);

// Compute regression
static void rgcomp(double x[16], rgparam_t * rgpr) {
	//				  { 0.106, 0.215, 0.324, 0.433, 0.542,  0.652, 0.78,   0.87,  0.0 }
	const double y[9] = { 31.07, 63.02, 94.96, 126.9, 158.86, 191.1, 228.62, 255.0, 0.0 };

	polyreg(REGORDER, 9, x, y, rgpr->cf);
}

// Convert a value to 0-255 based off the provided regression curve
static double rgcal(float x, rgparam_t *rgpr) {
	double y, p;
	int i;

	for (i = 0, y = 0.0, p = 1.0; i < REGORDER + 1; i++) {
		y += rgpr->cf[i] * p;
		p = p * x;
	}
	return y;
}

static double tele[16];
static double Cs;

void histogramEqualise(float **prow, int nrow, int offset, int width){
	// Plot histogram
	int histogram[256] = { 0 };
	for(int y = 0; y < nrow; y++)
		for(int x = 0; x < width; x++)
			histogram[(int)CLIP(prow[y][x+offset], 0, 255)]++;

	// Calculate cumulative frequency
	long sum = 0, cf[256] = { 0 };
	for(int i = 0; i < 255; i++){
		sum += histogram[i];
		cf[i] = sum;
	}

	// Apply histogram
	int area = nrow * width;
	for(int y = 0; y < nrow; y++){
		for(int x = 0; x < width; x++){
			int k = prow[y][x+offset];
			prow[y][x+offset] = (256.0/area) * cf[k];
		}
	}
}

void linearEnhance(float **prow, int nrow, int offset, int width){
	// Plot histogram
	int histogram[256] = { 0 };
	for(int y = 0; y < nrow; y++)
		for(int x = 0; x < width; x++)
			histogram[(int)CLIP(prow[y][x+offset], 0, 255)]++;

	// Find min/max points
	int min = -1, max = -1;
	for(int i = 5; i < 250; i++){
		if(histogram[i]/width/(nrow/255.0) > 0.25){
			if(min == -1) min = i;
			max = i;
		}
	}

	// Stretch the brightness into the new range
	for(int y = 0; y < nrow; y++)
		for(int x = 0; x < width; x++)
			prow[y][x+offset] = (prow[y][x+offset]-min) / (max-min) * 255.0;
}

// Brightness calibrate, including telemetry
void calibrateImage(float **prow, int nrow, int offset, int width, rgparam_t regr){
	offset -= SYNC_WIDTH+SPC_WIDTH;

	for (int y = 0; y < nrow; y++) {
		for (int x = 0; x < width+SYNC_WIDTH+SPC_WIDTH+TELE_WIDTH; x++) {
			float pv = rgcal(prow[y][x + offset], &regr);
			prow[y][x + offset] = CLIP(pv, 0, 255);
		}
	}
}

double teleNoise(double wedges[16]){
	double pattern[9] = { 31.07, 63.02, 94.96, 126.9, 158.86, 191.1, 228.62, 255.0, 0.0 };
	double noise = 0;
	for(int i = 0; i < 9; i++)
		noise += fabs(wedges[i] - pattern[i]);

	return noise;
}

// Get telemetry data for thermal calibration/equalization
int calibrate(float **prow, int nrow, int offset, int width) {
	double teleline[MAX_HEIGHT] = { 0.0 };
	double wedge[16];
	rgparam_t regr[MAX_HEIGHT/FRAME_LEN + 1];
	int telestart, mtelestart = 0;
	int channel = -1;

	// The minimum rows required to decode a full frame
	if (nrow < 192) {
		fprintf(stderr, "Telemetry decoding error, not enough rows\n");
		return 0;
	}

	// Calculate average of a row of telemetry
	for (int y = 0; y < nrow; y++) {
		for (int x = 3; x < 43; x++)
			teleline[y] += prow[y][x + offset + width];

		teleline[y] /= 40.0;
	}

	/* Wedge 7 is white and 8 is black, this will have the largest
	 * difference in brightness, this can be used to find the current
	 * position within the telemetry.
	 */
	float max = 0.0f;
	for (int n = nrow / 3 - 64; n < 2 * nrow / 3 - 64; n++) {
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

	telestart = (mtelestart - 64) % FRAME_LEN;

	// Make sure that theres at least one full frame in the image
	if (nrow < telestart + FRAME_LEN) {
		fprintf(stderr, "Telemetry decoding error, not enough rows\n");
		return 0;
	}

	// Find the least noisy frame
	double minNoise = -1;
	int bestFrame = -1;
	 for (int n = telestart, k = 0; n < nrow - FRAME_LEN; n += FRAME_LEN, k++) {
		int j;

		for (j = 0; j < 16; j++) {
			int i;

			wedge[j] = 0.0;
			for (i = 1; i < 7; i++)
				wedge[j] += teleline[n + j * 8 + i];
			wedge[j] /= 6;
		}

		double noise = teleNoise(wedge);
		if(noise < minNoise || minNoise == -1){
			minNoise = noise;
			bestFrame = k;

			// Compute & apply regression on the wedges
			rgcomp(wedge, &regr[k]);
			for (int j = 0; j < 16; j++)
				tele[j] = rgcal(wedge[j], &regr[k]);

			/* Compare the channel ID wedge to the reference
			 * wedges, the wedge with the closest match will
			 * be the channel ID
			 */
			float min = -1;
			for (int j = 0; j < 6; j++) {
				float df = tele[15] - tele[j];
				df *= df;

				if (df < min || min == -1) {
					channel = j;
					min = df;
				}
			}

			// Find the brightness of the minute marker, I don't really know what for
			Cs = 0.0;
			int i, j = n;
			for (i = 0, j = n; j < n + FRAME_LEN; j++) {
				float csline = 0.0;
				for (int l = 3; l < 43; l++)
					csline += prow[n][l + offset - SPC_WIDTH];
				csline /= 40.0;

				if (csline > 50.0) {
					Cs += csline;
					i++;
				}
			}
			Cs = rgcal(Cs / i, &regr[k]);
		}
	}

	if(bestFrame == -1){
		fprintf(stderr, "Something has gone very wrong, please file a bug report.");
		return 0;
	}

	calibrateImage(prow, nrow, offset, width, regr[bestFrame]);

	return channel + 1;

}

void distrib(options_t *opts, image_t *img, char *chid) {
	int max = 0;

	// Options
	options_t options;
	options.path = opts->path;
	options.effects = "";
	options.map = "";

	// Image options
	image_t distrib;
	strcpy(distrib.name, img->name);
	distrib.nrow = 256;

	// Assign memory
	for(int i = 0; i < 256; i++)
		distrib.prow[i] = (float *) malloc(sizeof(float) * 256);

	for(int n = 0; n < img->nrow; n++) {
		float *pixelv = img->prow[n];

		for(int i = 0; i < CH_WIDTH; i++) {
			int y = CLIP((int)pixelv[i + CHA_OFFSET], 0, 255);
			int x = CLIP((int)pixelv[i + CHB_OFFSET], 0, 255);
			distrib.prow[y][x]++;
			if(distrib.prow[y][x] > max)
				max = distrib.prow[y][x];
		}
	}

	// Scale to 0-255
	for(int x = 0; x < 256; x++)
		for(int y = 0; y < 256; y++)
			distrib.prow[y][x] = distrib.prow[y][x] / max * 255.0;

	extern int ImageOut(options_t *opts, image_t *img, int offset, int width, char *desc, char *chid, char *palette);
	ImageOut(&options, &distrib, 0, 256, "Distribution", chid, NULL);
}

extern float quick_select(float arr[], int n);

// Biased median denoise, pretyt ugly
#define TRIG_LEVEL 40
void denoise(float **prow, int nrow, int offset, int width){
	for(int y = 2; y < nrow-2; y++){
		for(int x = offset+1; x < offset+width-1; x++){
			if(prow[y][x+1] - prow[y][x] > TRIG_LEVEL ||
			   prow[y][x-1] - prow[y][x] > TRIG_LEVEL ||
			   prow[y+1][x] - prow[y][x] > TRIG_LEVEL ||
			   prow[y-1][x] - prow[y][x] > TRIG_LEVEL){
				prow[y][x] = quick_select((float[]){
					prow[y+2][x-1], prow[y+2][x], prow[y+2][x+1],
					prow[y+1][x-1], prow[y+1][x], prow[y+1][x+1],
					prow[y-1][x-1], prow[y-1][x], prow[y-1][x+1],
					prow[y-2][x-1], prow[y-2][x], prow[y-2][x+1]
				}, 12);
			}
		}
	}
}
#undef TRIG_LEVEL

// Flips a channe, for southbound passes
void flipImage(image_t *img, int width, int offset){
	for(int y = 1; y < img->nrow; y++){
		for(int x = 1; x < ceil(width / 2.0); x++){
			// Flip top-left & bottom-right
			float buffer = img->prow[img->nrow - y][offset + x];
			img->prow[img->nrow - y][offset + x] = img->prow[y][offset + (width - x)];
			img->prow[y][offset + (width - x)] = buffer;
		}
	}
}

// --- Temperature Calibration --- //
#include "satcal.h"

typedef struct {
	double Nbb;
	double Cs;
	double Cb;
	int ch;
} tempparam_t;

// IR channel temperature compensation
static void tempcomp(double t[16], int ch, int satnum, tempparam_t *tpr) {
	double Tbb, T[4];
	double C;

	tpr->ch = ch - 4;

	// Compute equivalent T blackbody temperature
	for (int n = 0; n < 4; n++) {
		float d0, d1, d2;

		C = t[9 + n] * 4.0;
		d0 = satcal[satnum].d[n][0];
		d1 = satcal[satnum].d[n][1];
		d2 = satcal[satnum].d[n][2];
		T[n] = d0;
		T[n] += d1 * C;
		C *= C;
		T[n] += d2 * C;
	}
	Tbb = (T[0] + T[1] + T[2] + T[3]) / 4.0;
	Tbb = satcal[satnum].rad[tpr->ch].A + satcal[satnum].rad[tpr->ch].B * Tbb;

	// Compute blackbody radiance temperature
	C = satcal[satnum].rad[tpr->ch].vc;
	tpr->Nbb = c1 * C * C * C / (expm1(c2 * C / Tbb));

	// Store blackbody count and space
	tpr->Cs = Cs * 4.0;
	tpr->Cb = t[14] * 4.0;
}

// IR channel temperature calibration
static double tempcal(float Ce, int satnum, tempparam_t * rgpr) {
	double Nl, Nc, Ns, Ne;
	double T, vc;

	Ns = satcal[satnum].cor[rgpr->ch].Ns;
	Nl = Ns + (rgpr->Nbb - Ns) * (rgpr->Cs - Ce * 4.0) / (rgpr->Cs - rgpr->Cb);
	Nc = satcal[satnum].cor[rgpr->ch].b[0] +
		 satcal[satnum].cor[rgpr->ch].b[1] * Nl +
		 satcal[satnum].cor[rgpr->ch].b[2] * Nl * Nl;

	Ne = Nl + Nc;

	vc = satcal[satnum].rad[rgpr->ch].vc;
	T = c2 * vc / log(c1 * vc * vc * vc / Ne + 1.0);
	T = (T - satcal[satnum].rad[rgpr->ch].A) / satcal[satnum].rad[rgpr->ch].B;

	// Convert to celsius
	T -= 273.15;
	// Rescale to 0-255 for -100°C to +60°C
	T = (T + 100.0) / 160.0 * 255.0;

	return T;
}

// Temperature calibration wrapper
void temperature(options_t *opts, image_t *img, int offset, int width){
	tempparam_t temp;

	printf("Temperature... ");
	fflush(stdout);

	tempcomp(tele, img->chB, opts->satnum - 15, &temp);

	for (int y = 0; y < img->nrow; y++) {
		for (int x = 0; x < width; x++) {
			img->prow[y][x + offset] = tempcal(img->prow[y][x + offset], opts->satnum - 15, &temp);
		}
	}
	printf("Done\n");
}