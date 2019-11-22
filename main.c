/*
 *  Aptdec
 *  Copyright (c) 2004-2005 by Thierry Leconte (F4DWV)
 *
 *      $Id$
 *
 *   This library is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details.
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>

#include <libgen.h>

#include <sndfile.h>
#include <png.h>

#include "messages.h"
#include "offsets.h"

#include "temppalette.h"
#include "gvipalette.h"

extern int getpixelrow(float *pixelv);
extern int init_dsp(double F);;

static SNDFILE *inwav;

static int initsnd(char *filename) {
    SF_INFO infwav;
    int	res;

	// Open audio file
    infwav.format = 0;
    inwav = sf_open(filename, SFM_READ, &infwav);
    if (inwav == NULL) {
		fprintf(stderr, ERR_FILE_READ, filename);
		return(0);
    }

    res = init_dsp(infwav.samplerate);
	printf("Input file: %s\n", filename);
    if(res < 0) {
		fprintf(stderr, "Input sample rate too low: %d\n", infwav.samplerate);
		return(0);
    }else if(res > 0) {
		fprintf(stderr, "Input sample rate too high: %d\n", infwav.samplerate);
		return(0);
    }
    printf("Input sample rate: %d\n", infwav.samplerate);

    if (infwav.channels != 1) {
		fprintf(stderr, "Too many channels in input file: %d\n", infwav.channels);
		return(0);
    }

    return(1);
}

// Get a sample from the wave file
int getsample(float *sample, int nb) {
    return(sf_read_float(inwav, sample, nb));
}

static png_text text_ptr[] = {
    {PNG_TEXT_COMPRESSION_NONE, "Software", VERSION},
    {PNG_TEXT_COMPRESSION_NONE, "Channel", NULL, 0},
    {PNG_TEXT_COMPRESSION_NONE, "Description", "NOAA satellite image", 20}
};

static int ImageOut(char *filename, char *chid, float **prow, int nrow, int width, int offset, png_color *palette, int croptele, int layered) {
    FILE *pngfile;
    png_infop info_ptr;
    png_structp png_ptr;

	// Reduce the width of the image to componsate for the missing telemetry
	if(croptele) width -= TOTAL_TELE;

	// Initalise the PNG writer
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
		fprintf(stderr, ERR_PNG_WRITE);
		return(1);
    }

	// Metadata
    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
		png_destroy_write_struct(&png_ptr, (png_infopp) NULL);
		fprintf(stderr, ERR_PNG_INFO);
		return(1);
    }

    if(palette == NULL) {
		// Greyscale image
    	png_set_IHDR(png_ptr, info_ptr, width, nrow,
					 8, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE,
				 	 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    } else {
		// Palleted color image
    	png_set_IHDR(png_ptr, info_ptr, width, nrow,
		 			 8, PNG_COLOR_TYPE_PALETTE, PNG_INTERLACE_NONE,
		 			 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
		png_set_PLTE(png_ptr, info_ptr, palette, 256);
    }

    text_ptr[1].text = chid;
    text_ptr[1].text_length = strlen(chid);
    png_set_text(png_ptr, info_ptr, text_ptr, 3);
    png_set_pHYs(png_ptr, info_ptr, 4000, 4000, PNG_RESOLUTION_METER);

    printf("Writing %s ", filename);
    fflush(stdout);
    pngfile = fopen(filename, "wb");
    if (pngfile == NULL) {
		fprintf(stderr, ERR_FILE_WRITE, filename);
		return(1);
    }
    png_init_io(png_ptr, pngfile);
    png_write_info(png_ptr, info_ptr);

    for (int n = 0; n < nrow; n++) {
		float *pixelv;
		png_byte pixel[2*IMG_WIDTH];

		pixelv = prow[n];
		int f = 0;
		for (int i = 0; i < width; i++) {
			// Skip parts of the image that are telemtry
			if(croptele){
				switch (i) {
					case 0:
						f += SYNC_WIDTH + SPC_WIDTH;
						break;
					case CH_WIDTH:
						f += TELE_WIDTH + SYNC_WIDTH + SPC_WIDTH;
						break;
					case CH_WIDTH*2:
						f += TELE_WIDTH;
				}
			}

			if(layered){
				// Layered image, basically overlay highlights in channel B over channel A
				float cloud = CLIP(pixelv[i+CHB_OFFSET]-141, 0, 255)/114*255;
				pixel[i] = CLIP(pixelv[i+CHA_OFFSET] + cloud, 0, 255);
			}else{
				pixel[i] = pixelv[i + offset + f];
			}
		}
		png_write_row(png_ptr, pixel);

    }
    png_write_end(png_ptr, info_ptr);
    fclose(pngfile);
    printf("\nDone\n");
    png_destroy_write_struct(&png_ptr, &info_ptr);
    return(0);
}

static int ImageRGBOut(char *filename, float **prow, int nrow) {
    FILE *pngfile;
    png_infop info_ptr;
    png_structp png_ptr;

    extern void falsecolor(double v, double t, float *r, float *g, float *b);

	// Initalise the PNG writer
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
		fprintf(stderr, ERR_PNG_WRITE);
		return(1);
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
		png_destroy_write_struct(&png_ptr, (png_infopp) NULL);
		fprintf(stderr, ERR_PNG_WRITE);
		return(1);
    }

    png_set_IHDR(png_ptr, info_ptr, CH_WIDTH, nrow,
		 		 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
		 		 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_set_pHYs(png_ptr, info_ptr, 4000, 4000, PNG_RESOLUTION_METER);

    text_ptr[1].text = "False Color";
    text_ptr[1].text_length = strlen(text_ptr[1].text);
    png_set_text(png_ptr, info_ptr, text_ptr, 3);

    printf("Computing false color & writing: %s... ", filename);
    fflush(stdout);
    pngfile = fopen(filename, "wb");
    if (pngfile == NULL) {
		fprintf(stderr, ERR_FILE_WRITE, filename);
		return(1);
    }
    png_init_io(png_ptr, pngfile);
    png_write_info(png_ptr, info_ptr);

    for (int n = 0; n < nrow ; n++) {
		png_color pix[CH_WIDTH];
		float *pixelc;

		pixelc = prow[n];

		for (int i = 0; i < CH_WIDTH - 1; i++) {
			float v, t;
			float r, g, b;

			v = pixelc[i+CHA_OFFSET];
			t = pixelc[i+CHB_OFFSET];

			falsecolor(v, t, &r, &g, &b);

			pix[i].red = 255.0 * r; 
			pix[i].green = 255.0 * g;
			pix[i].blue = 255.0 * b;
		}
		png_write_row(png_ptr, (png_bytep) pix);
    }
    png_write_end(png_ptr, info_ptr);
    fclose(pngfile);
    printf("Done\n");
    png_destroy_write_struct(&png_ptr, &info_ptr);
    return(0);
}

// Distribution of values between Channel A and Channel B
static void Distrib(char *filename, float **prow, int nrow) {
	unsigned int distrib[256][256];
	int n;
	int x, y;
	int max = 0;
	FILE *df;

	for(y = 0; y < 256; y++)
		for(x = 0; x < 256; x++)
			distrib[y][x] = 0;

	for(n = 0; n < nrow; n++) {
		float *pixelv;
		int i;

		pixelv = prow[n];
		for(i = 0; i < CH_WIDTH; i++) {
			y = (int)(pixelv[i + CHA_OFFSET]);
			x = (int)(pixelv[i + CHB_OFFSET]);
			distrib[y][x] += 1;
			if(distrib[y][x] > max) max=distrib[y][x];
		}
	}
	df = fopen(filename,"w");

	printf("Writing %s\n",filename);

	fprintf(df,"P2\n#max %d\n",max);
	fprintf(df,"256 256\n255\n");
	for(y = 0; y < 256; y++)
		for(x = 0; x < 256; x++)
			fprintf(df, "%d\n", (int)((255.0 * (double)(distrib[y][x])) / (double)max));
	fclose(df);
}

extern int calibrate(float **prow, int nrow, int offset, int contrastBoost);
extern void Temperature(float **prow, int nrow, int ch, int offset);
extern int Ngvi(float **prow, int nrow);
extern void readfconf(char *file);
extern int optind;
extern char *optarg;

// Default to NOAA 19
int satnum = 4;

static void usage(void) {
    printf("Aptdec [options] audio files ...\n"
	"Options:\n"
	" -e [c|t]       Enhancements\n"
	"     c: Contrast enhance\n"
	"     t: Crop telemetry\n"
	" -i [r|a|b|c|t] Output image type\n"
	"     r: Raw\n"
	"     a: A channel\n"
	"     b: B channel\n"
	"     c: False color\n"
	"     t: Temperature\n"
	"     l: Layered\n"
	" -d <dir>       Image destination directory.\n"
	" -s [15-19]     Satellite number\n"
	" -c <file>      False color config file\n");
   	exit(1);
}

int main(int argc, char **argv) {
    char pngfilename[1024];
    char name[500];
    char pngdirname[500] = "";

	// Images to create, default to a channel A and channel B image with contrast enhancement and cropped telemetry
    char imgopt[20] = "ab";
	char enchancements[20] = "ct";

	// Image buffer
    float *prow[3000];
	int nrow;

	// Mapping between wedge value and channel ID
    char *chid[]   = { "?", "1", "2", "3A", "4", "5", "3B" };
	char *chname[] = { "unknown", "visble", "near-infrared", "near-infrared", "thermal-infrared", "thermal-infrared", "thermal-infrared" };
    
	// The active sensor in each channel
    int chA, chB;

	// Print version
    printf("%s\n", VERSION);

	// Print usage if there are no arguments
	if(argc == 1)
		usage();

	int c;
    while ((c = getopt(argc, argv, "c:d:i:s:e:")) != EOF) {
		switch (c) {
			// Output directory name
			case 'd':
				strcpy(pngdirname, optarg);
				break;
			// False color config file
			case 'c':
				readfconf(optarg);
				break;
			// Output image type
			case 'i':
				strcpy(imgopt, optarg);
				break;
			// Satellite number (for calibration)
			case 's':
				satnum = atoi(optarg)-15;
				// Check if it's within the valid range
				if (satnum < 0 || satnum > 4) {
					fprintf(stderr, "Invalid satellite number, it must be the range [15-19]\n");
					exit(1);
				}
				break;
			// Enchancements
			case 'e':
				strcpy(enchancements, optarg);
				break;
			default:
				usage();
		}
    }

	if(optind == argc){
		printf("No audio files provided.\n");
		usage();
	}

	// Pull this in from filtercoeff.h
	extern float Sync[32];

	// Loop through the provided files
	for (; optind < argc; optind++) {
		chA = chB = 0;

		// Generate output name
		strcpy(pngfilename, argv[optind]);
		strcpy(name, basename(pngfilename));
		strtok(name, ".");

		if (pngdirname[0] == '\0')
			strcpy(pngdirname, dirname(pngfilename));

		// Open sound file, exit if that fails
		if (initsnd(argv[optind]) == 0)
			exit(1);

		// Main image building loop
		for (nrow = 0; nrow < 3000; nrow++) {
			// Allocate 2150 floats worth of memory for every line of the image
			prow[nrow] = (float *) malloc(sizeof(float) * 2150);
			
			// Read into prow and break the loop once we reach the end of the image
			if (getpixelrow(prow[nrow]) == 0)
				break;

			// Signal level
			double signal = 0;
			for (int i = 0; i < 32; i++) signal += prow[nrow][i] - Sync[i];
			
			// Signal level bar
			char bar[30];
			for (int i = 0; i < 30; i++) {
				bar[i] = ' ';
				if(i < signal/2000*30) bar[i] = '#';
			}

			printf("Row: %d, Signal Strength: %s\r", nrow, bar);
			fflush(stdout);
		}
		printf("\nTotal rows: %d\n", nrow);

		sf_close(inwav);

		// Layered images require contrast enhancements
		int contrastBoost = CONTAINS(enchancements, 'c') || CONTAINS(imgopt, 'l');

		chA = calibrate(prow, nrow, CHA_OFFSET, 0);
		chB = calibrate(prow, nrow, CHB_OFFSET, 0);
		printf("Channel A: %s (%s)\n", chid[chA], chname[chA]);
		printf("Channel B: %s (%s)\n", chid[chB], chname[chB]);

		// Temperature
		if (CONTAINS(imgopt, 't') && chB >= 4) {
			Temperature(prow, nrow, chB, CHB_OFFSET);
			sprintf(pngfilename, "%s/%s-t.png", pngdirname, name);
			ImageOut(pngfilename, "Temperature", prow, nrow, CH_WIDTH, CHB_OFFSET, (png_color*)TempPalette, 0, 0);
		}

		// We have to run the contrast enhance here because the temperature function requires real data
		// Yes, this is bodgy, yes I should replace this with something more elegant
		chA = calibrate(prow, nrow, CHA_OFFSET, contrastBoost);
		chB = calibrate(prow, nrow, CHB_OFFSET, contrastBoost);

		// Layered
		if (CONTAINS(imgopt, 'l')){
			sprintf(pngfilename, "%s/%s-l.png", pngdirname, name);
			ImageOut(pngfilename, "Layered", prow, nrow, CH_WIDTH, 0, NULL, 0, 1);
		}

		// Raw image
		if (CONTAINS(imgopt, 'r')) {
			int croptele = CONTAINS(enchancements, 't');
			char channelstr[45];
			sprintf(channelstr, "%s (%s) & %s (%s)", chid[chA], chname[chA], chid[chB], chname[chB]);

			sprintf(pngfilename, "%s/%s-r.png", pngdirname, name);
			ImageOut(pngfilename, channelstr, prow, nrow, IMG_WIDTH, 0, NULL, croptele, 0);
		}

		// Channel A
		if (CONTAINS(imgopt, 'a')) {
			char channelstr[21];
			sprintf(channelstr, "%s (%s)", chid[chA], chname[chA]);

			sprintf(pngfilename, "%s/%s-%s.png", pngdirname, name, chid[chA]);
			ImageOut(pngfilename, channelstr, prow, nrow, CH_WIDTH, CHA_OFFSET, NULL, 0, 0);
		}

		// Channel B
		if (CONTAINS(imgopt, 'b')) {
			char channelstr[21];
			sprintf(channelstr, "%s (%s)", chid[chB], chname[chB]);

			sprintf(pngfilename, "%s/%s-%s.png", pngdirname, name, chid[chB]);
			ImageOut(pngfilename, channelstr, prow, nrow, CH_WIDTH , CHB_OFFSET, NULL, 0, 0);
		}

		// Distribution map
		if (CONTAINS(imgopt, 'd')) {
			sprintf(pngfilename, "%s/%s-d.pnm", pngdirname, name);
			Distrib(pngfilename, prow, nrow);
		}

		// False color image
		if(CONTAINS(imgopt, 'c')){
			if (chA == 2 && chB == 4) { // Normal false color
				sprintf(pngfilename, "%s/%s-c.png", pngdirname, name);
				ImageRGBOut(pngfilename, prow, nrow);
			} else if (chA == 1 && chB == 2) { // GVI false color
				Ngvi(prow, nrow);
				sprintf(pngfilename, "%s/%s-c.png", pngdirname, name);
				ImageOut(pngfilename, "GVI False Color", prow, nrow, CH_WIDTH, CHB_OFFSET, (png_color*)GviPalette, 0, 0);
			} else {
				fprintf(stderr, "Lacking channels required for false color computation.\n");
			}
		}

		// Free the allocated memory
		for(int i = 0; i < 3000; i++) free(prow[i]);
	}

    exit(0);
}
