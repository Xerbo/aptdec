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
    float r, g, b;
} rgb_t;

extern rgb_t RGBcomposite(rgb_t top, float top_a, rgb_t bottom, float bottom_a);

static struct {
    rgb_t Sea, Land, Cloud;
    int Seaintensity, Seaoffset;
    int Landthreshold, Landintensity, Landoffset;
    int Cloudthreshold, Cloudintensity;
} fcinfo = {
    {28,  44,  95},
    {23,  78,  37},
    {240, 250, 255},
    50, 10,
    24, 34, 14,
    141, 114
};

// Read the config file
int readfcconf(char *file) {
    FILE *fin;
    fin = fopen(file, "r");

    if (fin == NULL)
	    return 0;

    fscanf(fin, "%g %g %g\n", &fcinfo.Sea.r,   &fcinfo.Sea.g,   &fcinfo.Sea.b);
    fscanf(fin, "%g %g %g\n", &fcinfo.Land.r,  &fcinfo.Land.g,  &fcinfo.Land.b);
    fscanf(fin, "%g %g %g\n", &fcinfo.Cloud.r, &fcinfo.Cloud.g, &fcinfo.Cloud.b);
    fscanf(fin, "%d\n",       &fcinfo.Seaintensity);
    fscanf(fin, "%d\n",       &fcinfo.Seaoffset);
    fscanf(fin, "%d\n",       &fcinfo.Landthreshold);
    fscanf(fin, "%d\n",       &fcinfo.Landintensity);
    fscanf(fin, "%d\n",       &fcinfo.Landoffset);
    fscanf(fin, "%d\n",       &fcinfo.Cloudthreshold);
    fscanf(fin, "%d",         &fcinfo.Cloudintensity);
    fclose(fin);

    return 1;
};

rgb_t falsecolor(float vis, float temp, float *r, float *g, float *b){
    rgb_t buffer;
    float land = 0, sea, cloud;

    // Calculate intensity of sea
    sea = CLIP(vis+fcinfo.Seaoffset, 0, fcinfo.Seaintensity)/fcinfo.Seaintensity;

    // Land
	if(vis > fcinfo.Landthreshold)
        land = CLIP(vis+fcinfo.Landoffset, 0, fcinfo.Landintensity)/fcinfo.Landintensity;
    
    // Composite land on sea
	buffer = RGBcomposite(fcinfo.Land, land, fcinfo.Sea, sea);

    // Composite clouds on top
	cloud = CLIP(temp-fcinfo.Cloudthreshold, 0, fcinfo.Cloudintensity)/fcinfo.Cloudintensity;
	buffer = RGBcomposite(fcinfo.Cloud, cloud, buffer, 1);

    return buffer;
}

// GVI (global vegetation index) false color
void Ngvi(float **prow, int nrow) {
    printf("Computing GVI false color");

    for (int n = 0; n < nrow; n++) {
        float *pixelv = prow[n];

        for (int i = 0; i < CH_WIDTH; i++) {
            double gvi = (pixelv[i + CHA_OFFSET] - pixelv[i + CHB_OFFSET])/
                         (pixelv[i + CHA_OFFSET] + pixelv[i + CHB_OFFSET]);

            gvi = (gvi + 0.1) * 340.0;
            pixelv[i + CHB_OFFSET] = CLIP(gvi, 0, 255);
        }
    }
    printf("\nDone\n");
};