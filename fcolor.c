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
#include <math.h>
#include "offsets.h"

typedef struct {
    float r, g, b, a;
} rgba;

static struct {
    rgba Sea;
    rgba Land;
    rgba Cloud;
    int Seaintensity;
    float Seaoffset;
    int Landthreshold;
    int Landintensity;
    int Landoffset;
    int Cloudthreshold;
    int Cloudintensity;
} fcinfo = {
    {28,  44,  95,  1},
    {23,  78,  37,  1},
    {240, 250, 255, 1},
    50,
    0.5,
    24,
    34,
    14,
    141,
    114
};

// Read the config file
void readfcconf(char *file) {
    FILE *fin;

    fin = fopen(file, "r");
    if (fin == NULL)
	    return;

    fscanf(fin, "%g %g %g\n", &fcinfo.Sea.r,   &fcinfo.Sea.g,   &fcinfo.Sea.b);
    fscanf(fin, "%g %g %g\n", &fcinfo.Land.r,  &fcinfo.Land.g,  &fcinfo.Land.b);
    fscanf(fin, "%g %g %g\n", &fcinfo.Cloud.r, &fcinfo.Cloud.g, &fcinfo.Cloud.b);
    fscanf(fin, "%d\n",       &fcinfo.Seaintensity);
    fscanf(fin, "%g\n",       &fcinfo.Seaoffset);
    fscanf(fin, "%d\n",       &fcinfo.Landthreshold);
    fscanf(fin, "%d\n",       &fcinfo.Landintensity);
    fscanf(fin, "%d\n",       &fcinfo.Landoffset);
    fscanf(fin, "%d\n",       &fcinfo.Cloudthreshold);
    fscanf(fin, "%d",         &fcinfo.Cloudintensity);

    fclose(fin);
};

// RGBA Composite
rgba composite(rgba top, rgba bottom){
    rgba composite;
	composite.r = MCOMPOSITE(top.r, top.a, bottom.r, bottom.a);
	composite.g = MCOMPOSITE(top.g, top.a, bottom.g, bottom.a);
	composite.b = MCOMPOSITE(top.b, top.a, bottom.b, bottom.a);
    composite.a = bottom.a == 1 || top.a == 1 ? 1 : (top.a+bottom.a)/2;
    return composite;
}

void falsecolor(float vis, float temp, float *r, float *g, float *b){
    rgba buffer;
    fcinfo.Land.a = 0.0f; fcinfo.Sea.a = 0.0f;

    // Calculate intensity of sea and land
    fcinfo.Sea.a  = CLIP(vis, 0, 20)/fcinfo.Seaintensity + fcinfo.Seaoffset;
	if(vis > fcinfo.Landthreshold) fcinfo.Land.a = CLIP(vis-fcinfo.Landoffset, 0, fcinfo.Landintensity)/fcinfo.Landintensity;

    // Composite land on top of sea
	buffer = composite(fcinfo.Land, fcinfo.Sea);
    buffer.a = 1.0f;

    // Composite clouds on top
	fcinfo.Cloud.a = CLIP(temp-fcinfo.Cloudthreshold, 0, fcinfo.Cloudintensity)/fcinfo.Cloudintensity;
	buffer = composite(fcinfo.Cloud, buffer);

    *r = buffer.r;
    *g = buffer.g;
    *b = buffer.b;
}

// GVI (global vegetation index) false color
void Ngvi(float **prow, int nrow) {
    printf("Computing GVI false color\n");
    fflush(stdout);

    for (int n = 0; n < nrow; n++) {
        float *pixelv = prow[n];
        for (int i = 0; i < CH_WIDTH; i++) {
            float pv;
	        double gvi;

            gvi = (pixelv[i + CHA_OFFSET] - pixelv[i + CHB_OFFSET]) / (pixelv[i + CHA_OFFSET] + pixelv[i + CHB_OFFSET]);

            pv = (gvi + 0.1) * 340.0;
            pv = CLIP(pv, 0, 255);

            pixelv[i + CHB_OFFSET] = pv;
        }
    }
};