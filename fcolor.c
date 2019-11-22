#include <stdio.h>
#include <math.h>
#include "offsets.h"

typedef struct {
    float h, s, v;
} hsvpix_t;

static void HSVtoRGB(float *r, float *g, float *b, hsvpix_t pix) {
    int i;
    double f, p, q, t, h;

    if (pix.s == 0) {
        // achromatic (grey)
        *r = *g = *b = pix.v;
        return;
    }

    h = pix.h / 60;		// sector 0 to 5
    i = floor(h);
    f = h - i;			// factorial part of h
    p = pix.v * (1 - pix.s);
    q = pix.v * (1 - pix.s * f);
    t = pix.v * (1 - pix.s * (1 - f));

    switch (i) {
        case 0:
            *r = pix.v;
            *g = t;
            *b = p;
            break;
        case 1:
            *r = q;
            *g = pix.v;
            *b = p;
            break;
        case 2:
            *r = p;
            *g = pix.v;
            *b = t;
            break;
        case 3:
            *r = p;
            *g = q;
            *b = pix.v;
            break;
        case 4:
            *r = t;
            *g = p;
            *b = pix.v;
            break;
        default:			// case 5:
            *r = pix.v;
            *g = p;
            *b = q;
            break;
    }

}

static struct {
    float Seathreshold;
    float Landthreshold;
    float Threshold;
    hsvpix_t CloudTop;
    hsvpix_t CloudBot;
    hsvpix_t SeaTop;
    hsvpix_t SeaBot;
    hsvpix_t GroundTop;
    hsvpix_t GroundBot;
} fcinfo = {
    30.0, 90.0, 155.0, {
    230, 0.2, 0.3}, {
    230, 0.0, 1.0}, {
    200.0, 0.7, 0.6}, {
    240.0, 0.6, 0.4}, {
    60.0, 0.6, 0.2}, {
    100.0, 0.0, 0.5}
};

void readfconf(char *file) {
    FILE *fin;

    fin = fopen(file, "r");
    if (fin == NULL)
	    return;

    fscanf(fin, "%g\n", &fcinfo.Seathreshold);
    fscanf(fin, "%g\n", &fcinfo.Landthreshold);
    fscanf(fin, "%g\n", &fcinfo.Threshold);
    fscanf(fin, "%g %g %g\n", &fcinfo.CloudTop.h,  &fcinfo.CloudTop.s,  &fcinfo.CloudTop.v);
    fscanf(fin, "%g %g %g\n", &fcinfo.CloudBot.h,  &fcinfo.CloudBot.s,  &fcinfo.CloudBot.v);
    fscanf(fin, "%g %g %g\n", &fcinfo.SeaTop.h,    &fcinfo.SeaTop.s,    &fcinfo.SeaTop.v);
    fscanf(fin, "%g %g %g\n", &fcinfo.SeaBot.h,    &fcinfo.SeaBot.s,    &fcinfo.SeaBot.v);
    fscanf(fin, "%g %g %g\n", &fcinfo.GroundTop.h, &fcinfo.GroundTop.s, &fcinfo.GroundTop.v);
    fscanf(fin, "%g %g %g\n", &fcinfo.GroundBot.h, &fcinfo.GroundBot.s, &fcinfo.GroundBot.v);

    fclose(fin);
};

void falsecolor(double v, double t, float *r, float *g, float *b) {
    hsvpix_t top, bot, c;
    double scv, sct;

    if (t > fcinfo.Threshold) {
        if (v < fcinfo.Seathreshold) {
            // Sea
            top = fcinfo.SeaTop, bot = fcinfo.SeaBot;
            scv = v / fcinfo.Seathreshold;
            sct = (256.0 - t) / (256.0 - fcinfo.Threshold);
        } else {
            // Ground
            top = fcinfo.GroundTop, bot = fcinfo.GroundBot;
            scv = (v - fcinfo.Seathreshold) / (fcinfo.Landthreshold - fcinfo.Seathreshold);
            sct = (256.0 - t) / (256.0 - fcinfo.Threshold);
        }
    } else {
        // Clouds
        top = fcinfo.CloudTop, bot = fcinfo.CloudBot;
        scv = v / 256.0;
        sct = (256.0 - t) / 256.0;
    }

    c.s = top.s + sct * (bot.s - top.s);
    c.v = top.v + scv * (bot.v - top.v);
    c.h = top.h + scv * sct * (bot.h - top.h);

    HSVtoRGB(r, g, b, c);
};

void Ngvi(float **prow, int nrow) {
    printf("GVI... ");
    fflush(stdout);

    for (int n = 0; n < nrow; n++) {
        float *pixelv;

        pixelv = prow[n];
        for (int i = 0; i < CH_WIDTH; i++) {
            float pv;
	        double gvi;

            gvi = (pixelv[i + CHA_OFFSET] - pixelv[i + CHB_OFFSET]) / (pixelv[i + CHA_OFFSET] + pixelv[i + CHB_OFFSET]);

            pv = (gvi + 0.1) * 340.0;
            pv = CLIP(pv, 0, 255);

            pixelv[i + CHB_OFFSET] = pv;
        }
    }
    printf("Done\n");
};

