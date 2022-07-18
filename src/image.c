/* 
 *  This file is part of Aptdec.
 *  Copyright (c) 2004-2009 Thierry Leconte (F4DWV), Xerbo (xerbo@protonmail.com) 2019-2022
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
#include <math.h>
#include <stdlib.h>

#include "apt.h"
#include "algebra.h"
#include "image.h"

static linear_t compute_regression(float *wedges) {
	//				    { 0.106, 0.215, 0.324, 0.433, 0.542,  0.652, 0.78,   0.87,  0.0 }
	const float teleramp[9] = { 31.07, 63.02, 94.96, 126.9, 158.86, 191.1, 228.62, 255.0, 0.0 };

	return linear_regression(wedges, teleramp, 9);
}

static float tele[16];
static float Cs;

void apt_histogramEqualise(float **prow, int nrow, int offset, int width){
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
			int k = (int)prow[y][x+offset];
			prow[y][x+offset] = (256.0f/area) * cf[k];
		}
	}
}

void apt_linearEnhance(float **prow, int nrow, int offset, int width){
	// Plot histogram
	int histogram[256] = { 0 };
	for(int y = 0; y < nrow; y++)
		for(int x = 0; x < width; x++)
			histogram[(int)CLIP(prow[y][x+offset], 0, 255)]++;

	// Find min/max points
	int min = -1, max = -1;
	for(int i = 5; i < 250; i++){
		if(histogram[i]/width/(nrow/255.0) > 0.1){
			if(min == -1) min = i;
			max = i;
		}
	}

	// Stretch the brightness into the new range
	for(int y = 0; y < nrow; y++){
		for(int x = 0; x < width; x++){
			prow[y][x+offset] = (prow[y][x+offset]-min) / (max-min) * 255.0f;
			prow[y][x+offset] = CLIP(prow[y][x+offset], 0.0f, 255.0f);
		}
	}
}

// Brightness calibrate, including telemetry
void calibrateImage(float **prow, int nrow, int offset, int width, linear_t regr){
	offset -= APT_SYNC_WIDTH+APT_SPC_WIDTH;

	for (int y = 0; y < nrow; y++) {
		for (int x = 0; x < width+APT_SYNC_WIDTH+APT_SPC_WIDTH+APT_TELE_WIDTH; x++) {
			float pv = linear_calc(prow[y][x + offset], regr);
			prow[y][x + offset] = CLIP(pv, 0, 255);
		}
	}
}

float teleNoise(float *wedges){
	float pattern[9] = { 31.07, 63.02, 94.96, 126.9, 158.86, 191.1, 228.62, 255.0, 0.0 };
	float noise = 0;
	for(int i = 0; i < 9; i++)
		noise += fabs(wedges[i] - pattern[i]);

	return noise;
}

// Get telemetry data for thermal calibration
apt_channel_t apt_calibrate(float **prow, int nrow, int offset, int width) {
	float teleline[APT_MAX_HEIGHT] = { 0.0 };
	float wedge[16];
	linear_t regr[APT_MAX_HEIGHT/APT_FRAME_LEN + 1];
	int telestart, mtelestart = 0;
	int channel = -1;

	// The minimum rows required to decode a full frame
	if (nrow < APT_CALIBRATION_ROWS) {
		fprintf(stderr, "Telemetry decoding error, not enough rows\n");
		return APT_CHANNEL_UNKNOWN;
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

		// (sum 4px below) - (sum 4px above)
		df = (float)((teleline[n - 4] + teleline[n - 3] + teleline[n - 2] + teleline[n - 1]) -
			     (teleline[n + 0] + teleline[n + 1] + teleline[n + 2] + teleline[n + 3]));

		// Find the maximum difference
		if (df > max) {
			mtelestart = n;
			max = df;
		}
	}

	telestart = (mtelestart + 64) % APT_FRAME_LEN;

	// Make sure that theres at least one full frame in the image
	if (nrow < telestart + APT_FRAME_LEN) {
		fprintf(stderr, "Telemetry decoding error, not enough rows\n");
		return APT_CHANNEL_UNKNOWN;
	}

	// Find the least noisy frame
	float minNoise = -1;
	int bestFrame = -1;
	 for (int n = telestart, k = 0; n < nrow - APT_FRAME_LEN; n += APT_FRAME_LEN, k++) {
		int j;

		for (j = 0; j < 16; j++) {
			int i;

			wedge[j] = 0.0;
			for (i = 1; i < 7; i++)
				wedge[j] += teleline[n + j * 8 + i];
			wedge[j] /= 6;
		}

		float noise = teleNoise(wedge);
		if(noise < minNoise || minNoise == -1){
			minNoise = noise;
			bestFrame = k;

			// Compute & apply regression on the wedges
			regr[k] = compute_regression(wedge);
			for (int j = 0; j < 16; j++)
				tele[j] = linear_calc(wedge[j], regr[k]);

			/* Compare the channel ID wedge to the reference
			 * wedges, the wedge with the closest match will
			 * be the channel ID
			 */
			float min = -1;
			for (int j = 0; j < 6; j++) {
				float df = (float)(tele[15] - tele[j]);
				df *= df;

				if (df < min || min == -1) {
					channel = j;
					min = df;
				}
			}

			// Find the brightness of the minute marker, I don't really know what for
			Cs = 0.0;
			int i, j = n;
			for (i = 0, j = n; j < n + APT_FRAME_LEN; j++) {
				float csline = 0.0;
				for (int l = 3; l < 43; l++)
					csline += prow[n][l + offset - APT_SPC_WIDTH];
				csline /= 40.0;

				if (csline > 50.0) {
					Cs += csline;
					i++;
				}
			}
			Cs = linear_calc((Cs / i), regr[k]);
		}
	}

	if(bestFrame == -1){
		fprintf(stderr, "Something has gone very wrong, please file a bug report.\n");
		return APT_CHANNEL_UNKNOWN;
	}

	calibrateImage(prow, nrow, offset, width, regr[bestFrame]);

	return (apt_channel_t)(channel + 1);

}

extern float quick_select(float arr[], int n);

// Biased median denoise, pretyt ugly
#define TRIG_LEVEL 40
void apt_denoise(float **prow, int nrow, int offset, int width){
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

// Flips a channel, for northbound passes
void apt_flipImage(apt_image_t *img, int width, int offset){
	for(int y = 1; y < img->nrow; y++){
		for(int x = 1; x < ceil(width / 2.0); x++){
			// Flip top-left & bottom-right
			float buffer = img->prow[img->nrow - y][offset + x];
			img->prow[img->nrow - y][offset + x] = img->prow[y][offset + (width - x)];
			img->prow[y][offset + (width - x)] = buffer;
		}
	}
}

// Calculate crop to reomve noise from the start and end of an image
int apt_cropNoise(apt_image_t *img){
	#define NOISE_THRESH 180.0

	// Average value of minute marker
	float spc_rows[APT_MAX_HEIGHT] = { 0.0 };
	int startCrop = 0; int endCrop = img->nrow;
	for(int y = 0; y < img->nrow; y++) {
		for(int x = 0; x < APT_SPC_WIDTH; x++) {
			spc_rows[y] += img->prow[y][x + (APT_CHB_OFFSET - APT_SPC_WIDTH)];
		}
		spc_rows[y] /= APT_SPC_WIDTH;

		// Skip minute markings
		if(spc_rows[y] < 10) {
			spc_rows[y] = spc_rows[y-1];
		}
	}

	// 3 row average
	for(int y = 0; y < img->nrow; y++){
		spc_rows[y] = (spc_rows[y+1] + spc_rows[y+2] + spc_rows[y+3])/3;
		//img.prow[y][0] = spc_rows[y];
	}

	// Find ends
	for(int y = 0; y < img->nrow-1; y++) {
		if(spc_rows[y] > NOISE_THRESH){
			endCrop = y;
		}
	}
	for(int y = img->nrow; y > 0; y--) {
		if(spc_rows[y] > NOISE_THRESH) {
			startCrop = y;
		}
	}

	//printf("Crop rows: %i -> %i\n", startCrop, endCrop);

	// Remove the noisy rows at start
	for(int y = 0; y < img->nrow-startCrop; y++) {
		memmove(img->prow[y], img->prow[y+startCrop], sizeof(float)*APT_PROW_WIDTH);
	}

	// Ignore the noisy rows at the end
	img->nrow = (endCrop - startCrop);

	return startCrop;
}

// --- Temperature Calibration --- //
#include "satcal.h"

typedef struct {
	float Nbb;
	float Cs;
	float Cb;
	int ch;
} tempparam_t;

// IR channel temperature compensation
static void tempcomp(float t[16], int ch, int satnum, tempparam_t *tpr) {
	float Tbb, T[4];
	float C;

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
static float tempcal(float Ce, int satnum, tempparam_t * rgpr) {
	float Nl, Nc, Ns, Ne;
	float T, vc;

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
void apt_temperature(int satnum, apt_image_t *img, int offset, int width){
	tempparam_t temp;

	tempcomp(tele, img->chB, satnum - 15, &temp);

	for (int y = 0; y < img->nrow; y++) {
		for (int x = 0; x < width; x++) {
			img->prow[y][x + offset] = (float)tempcal(img->prow[y][x + offset], satnum - 15, &temp);
		}
	}
}
