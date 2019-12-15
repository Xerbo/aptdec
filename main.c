/* 
 *  This file is part of Aptdec.
 *  Copyright (c) 2004-2009 Thierry Leconte (F4DWV), Xerbo (xerbo@protonmail.com) 2019
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
#include <getopt.h>
#include <libgen.h>
#include <math.h>
#include <sndfile.h>
#include <png.h>

#include "messages.h"
#include "offsets.h"
#include "palette.h"

extern int getpixelrow(float *pixelv);
extern int init_dsp(double F);

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

// Get samples from the wave file
int getsample(float *sample, int nb) {
    return sf_read_float(inwav, sample, nb);
}

static png_text text_ptr[] = {
    {PNG_TEXT_COMPRESSION_NONE, "Software", VERSION},
    {PNG_TEXT_COMPRESSION_NONE, "Channel", NULL, 0},
    {PNG_TEXT_COMPRESSION_NONE, "Description", "NOAA satellite image", 20}
};

// TODO: this function needs to be tidied up
/* Effects
 *  0 - Nothing
 *  1 - Crop telemetry
 *  2 - False color
 *  3 - Layered
 */
static int ImageOut(char *filename, char *chid, float **prow, int nrow, int width, int offset, png_color *palette, int effects) {
    FILE *pngfile;
    png_infop info_ptr;
    png_structp png_ptr;

	// Reduce the width of the image to componsate for the missing telemetry
	if(effects == 1) width -= TOTAL_TELE;

	// Initalise the PNG writer
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
		fprintf(stderr, ERR_PNG_WRITE);
		return(0);
    }

	// Metadata
    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
		png_destroy_write_struct(&png_ptr, (png_infopp) NULL);
		fprintf(stderr, ERR_PNG_INFO);
		return(0);
    }

	extern void falsecolor(float vis, float temp, float *r, float *g, float *b);

    if(effects == 2){
		// 8 bit RGB image
		png_set_IHDR(png_ptr, info_ptr, width, nrow,
		 		 	 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
		 			 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	}else if(palette == NULL) {
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

	if(effects == 2){
		printf("Computing false color & writing: %s", filename);
	}else{
		printf("Writing %s", filename);
	}
    fflush(stdout);

    pngfile = fopen(filename, "wb");
    if (pngfile == NULL) {
		fprintf(stderr, ERR_FILE_WRITE, filename);
		return(1);
    }
    png_init_io(png_ptr, pngfile);
    png_write_info(png_ptr, info_ptr);

    for (int n = 0; n < nrow; n++) {
		png_color pix[width];
		png_byte pixel[width];

		float *pixelv = prow[n];
		int f = 0;
		for (int i = 0; i < width; i++) {
			// Skip parts of the image that are telemetry
			if(effects == 1){
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

			if(effects == 2){
				float r = 0, g = 0, b = 0;
				falsecolor(pixelv[i+CHA_OFFSET], pixelv[i+CHB_OFFSET], &r, &g, &b);
				pix[i].red = r;
				pix[i].green = g;
				pix[i].blue = b;
			}else if(effects == 3){
				// Layered image, overlay clouds in channel B over channel A
				float cloud = CLIP(pixelv[i+CHB_OFFSET]-141, 0, 255)/114;
				pixel[i] = MCOMPOSITE(255, cloud, pixelv[i+CHA_OFFSET], 1);
			}else{
				pixel[i] = pixelv[i + offset + f];
			}
		}

		if(effects == 2){
			png_write_row(png_ptr, (png_bytep) pix);
		}else{
			png_write_row(png_ptr, pixel);
		}
    }

    png_write_end(png_ptr, info_ptr);
    fclose(pngfile);
    printf("\nDone\n");
    png_destroy_write_struct(&png_ptr, &info_ptr);

    return(1);
}

// Outputs a image with the value distribution between channel A and B
static void distrib(char *filename, float **prow, int nrow) {
	float *distrib[256];
	int max = 0;

	// Assign memory
	for(int i = 0; i < 256; i++)
		distrib[i] = (float *) malloc(sizeof(float) * 256);

	for(int n = 0; n < nrow; n++) {
		float *pixelv = prow[n];
		
		for(int i = 0; i < CH_WIDTH; i++) {
			int y = (int)(pixelv[i + CHA_OFFSET]);
			int x = (int)(pixelv[i + CHB_OFFSET]);
			distrib[y][x] += 1;
			if(distrib[y][x] > max) max = distrib[y][x];
		}
	}

	// Scale to 0-255
	for(int x = 0; x < 256; x++)
		for(int y = 0; y < 256; y++)
			distrib[y][x] = distrib[y][x] / max * 255;


	ImageOut(filename, "Value distribution", distrib, 256, 256, 0, NULL, 0);
}

extern int calibrate(float **prow, int nrow, int offset, int width, int contrastEqualise);
extern void temperature(float **prow, int nrow, int ch, int offset);
extern int Ngvi(float **prow, int nrow);
extern void readfcconf(char *file);
extern int optind;
extern char *optarg;

// Default to NOAA 19
int satnum = 4;

static void usage(void) {
    printf("Aptdec [options] audio files ...\n"
	"Options:\n"
	" -e [c|t]       Enhancements\n"
	"     c: Contrast equalise\n"
	"     t: Crop telemetry\n"
	" -i [r|a|b|c|t] Output image type\n"
	"     r: Raw\n"
	"     a: Channel A\n"
	"     b: Channel B\n"
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

	// Default to a raw image, with equalization and cropped telemetry
    char imgopt[20] = "r";
	char enchancements[20] = "ct";

	// Image buffer
    float *prow[3000];
	int nrow;

	// Mapping between telemetry wedge value and channel
	static struct {
		char *id[7];
		char *name[7];
	} ch = {
		{ "?", 		 "1", 	   "2", 			"3A", 		    "4", 				"5", 				"3B" },
		{ "unknown", "visble", "near-infrared", "mid-infrared", "thermal-infrared", "thermal-infrared", "mid-infrared" }
	};

	// The active sensor in each channel
    int chA, chB;

	// Print version
    printf(VERSION"\n");

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
				readfcconf(optarg);
				break;
			// Output image type
			case 'i':
				strncpy(imgopt, optarg, 20);
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
				strncpy(enchancements, optarg, 20);
				break;
			default:
				usage();
		}
    }
	if(optind == argc){
		printf("No audio files provided.\n");
		usage();
	}

	// Process the provided files
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


			printf("Row: %d\r", nrow);
			fflush(stdout);
		}
		printf("\nTotal rows: %d\n", nrow);

		sf_close(inwav);

		// Layered & false color images both need brightness equalization
		int contrastEqualise = CONTAINS(enchancements, 'c') || CONTAINS(imgopt, 'l') || CONTAINS(imgopt, 'c');

		chA = calibrate(prow, nrow, CHA_OFFSET, CH_WIDTH, 0);
		chB = calibrate(prow, nrow, CHB_OFFSET, CH_WIDTH, 0);
		printf("Channel A: %s (%s)\n", ch.id[chA], ch.name[chA]);
		printf("Channel B: %s (%s)\n", ch.id[chB], ch.name[chB]);

		// Temperature
		if (CONTAINS(imgopt, 't') && chB >= 4) {
			temperature(prow, nrow, chB, CHB_OFFSET);
			sprintf(pngfilename, "%s/%s-t.png", pngdirname, name);
			ImageOut(pngfilename, "Temperature", prow, nrow, CH_WIDTH, CHB_OFFSET, (png_color*)TempPalette, 0);
		}

		// Run the contrast equalise here because the temperature calibration requires raw data
		if(contrastEqualise)
			calibrate(prow, nrow, CHA_OFFSET, CH_WIDTH+TELE_WIDTH+SYNC_WIDTH+SPC_WIDTH+CH_WIDTH, 1);

		// Layered
		if (CONTAINS(imgopt, 'l')){
			if(chA == 1){
				sprintf(pngfilename, "%s/%s-l.png", pngdirname, name);
				ImageOut(pngfilename, "Layered", prow, nrow, CH_WIDTH, 0, NULL, 3);
			}else{
				fprintf(stderr, "Lacking channels required for generting a layered image.\n");
			}
		}

		// Raw image
		if (CONTAINS(imgopt, 'r')) {
			int croptele = CONTAINS(enchancements, 't');
			char channelstr[45];
			sprintf(channelstr, "%s (%s) & %s (%s)", ch.id[chA], ch.name[chA], ch.id[chB], ch.name[chB]);

			sprintf(pngfilename, "%s/%s-r.png", pngdirname, name);
			ImageOut(pngfilename, channelstr, prow, nrow, IMG_WIDTH, 0, NULL, croptele ? 1 : 0);
		}

		// Channel A
		if (CONTAINS(imgopt, 'a')) {
			char channelstr[21];
			sprintf(channelstr, "%s (%s)", ch.id[chA], ch.name[chA]);

			sprintf(pngfilename, "%s/%s-%s.png", pngdirname, name, ch.id[chA]);
			ImageOut(pngfilename, channelstr, prow, nrow, CH_WIDTH, CHA_OFFSET, NULL, 0);
		}

		// Channel B
		if (CONTAINS(imgopt, 'b')) {
			char channelstr[21];
			sprintf(channelstr, "%s (%s)", ch.id[chB], ch.name[chB]);

			sprintf(pngfilename, "%s/%s-%s.png", pngdirname, name, ch.id[chB]);
			ImageOut(pngfilename, channelstr, prow, nrow, CH_WIDTH , CHB_OFFSET, NULL, 0);
		}

		// Distribution image
		if (CONTAINS(imgopt, 'd')) {
			sprintf(pngfilename, "%s/%s-d.png", pngdirname, name);
			distrib(pngfilename, prow, nrow);
		}

		// False color image
		if(CONTAINS(imgopt, 'c')){
			if (chA == 2 && chB >= 4) { // Normal false color
				sprintf(pngfilename, "%s/%s-c.png", pngdirname, name);
				//ImageRGBOut(pngfilename, prow, nrow);
				ImageOut(pngfilename, "False Color", prow, nrow, CH_WIDTH, 0, NULL, 2);
			} else if (chB == 2) { // GVI (global vegetation index) false color
				Ngvi(prow, nrow);
				sprintf(pngfilename, "%s/%s-c.png", pngdirname, name);
				ImageOut(pngfilename, "GVI False Color", prow, nrow, CH_WIDTH, CHB_OFFSET, (png_color*)GviPalette, 0);
			} else {
				fprintf(stderr, "Lacking channels required for generating a false color image.\n");
			}
		}
	}

    exit(0);
}
