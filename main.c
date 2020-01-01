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

extern int getpixelrow(float *pixelv, int nrow, int *zenith);
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

int mapOverlay(char *filename, float **crow, int nrow, int zenith, int MCIR) {
	FILE *fp = fopen(filename, "rb");

	// Create reader
	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(!png) return 0;
	png_infop info = png_create_info_struct(png);
	if(!info) return 0;
	png_init_io(png, fp);

	// Read info from header
	png_read_info(png, info);
	int width = png_get_image_width(png, info);
	int height = png_get_image_height(png, info);
	png_byte color_type = png_get_color_type(png, info);
	png_byte bit_depth = png_get_bit_depth(png, info);

	// Check the image
	if(width != 1040){
		fprintf(stderr, "Map must be 1040px wide.\n");
		return 0;
	}else if(bit_depth != 16){
		fprintf(stderr, "Map must be 16 bit color.\n");
		return 0;
	}else if(color_type != PNG_COLOR_TYPE_RGB){
		fprintf(stderr, "Map must be RGB.\n");
		return 0;
	}else if(zenith > height/2 || nrow-zenith > height/2){
		fprintf(stderr, "WARNING: Map is too short to cover entire image\n");
	}

	// Create row buffers
	png_bytep *mapRows = NULL;
	mapRows = (png_bytep *) malloc(sizeof(png_bytep) * height);
	for(int y = 0; y < height; y++) mapRows[y] = (png_byte *) malloc(png_get_rowbytes(png, info));

	// Read image
	png_read_image(png, mapRows);

	// Tidy up
	fclose(fp);
	png_destroy_read_struct(&png, &info, NULL);

	int mapOffset = (height/2)-zenith;
	for(int y = 0; y < nrow; y++) {
		for(int x = 49; x < width - 82; x++){
			// Maps are 16 bit / channel
			png_bytep px = &mapRows[CLIP(y + mapOffset, 0, height)][x * 6];
			uint16_t r = (uint16_t)(px[0] << 8) | px[1];
			uint16_t g = (uint16_t)(px[2] << 8) | px[3];
			uint16_t b = (uint16_t)(px[4] << 8) | px[5];

			// Pointers to the current pixel in each channel
			float *cha = &crow[y][(x+36) * 3];
			float *chb = &crow[y][(x+CHB_OFFSET-49) * 3];

			// Fill in map
			if(MCIR){
				// Sea
				cha[0] = 42; cha[1] = 53; cha[2] = 105;

				// Land
				if(g > 128){
					float vegetation = (r-160) / 96.0;
					cha[0] = 120; cha[1] = vegetation*60.0 + 100; cha[2] = 95;
				}
			}

			// Color -> alpha: composite
			int composite = r + g + b;
			// Color -> alpha: flattern color depth
			float factor = (255 * 255 * 2.0) / composite;
			r *= factor; g *= factor; b *= factor;
			// Color -> alpha: convert black to alpha
			float alpha = CLIP(abs(0 - composite) / 65535.0, 0, 1);

			// Map overlay on channel A
			cha[0] = MCOMPOSITE(r/257, alpha, cha[0], 1);
			cha[1] = MCOMPOSITE(g/257, alpha, cha[1], 1);
			cha[2] = MCOMPOSITE(b/257, alpha, cha[2], 1);

			// Map overlay on channel B
			if(!MCIR){
				chb[0] = MCOMPOSITE(r/257, alpha, chb[0], 1);
				chb[1] = MCOMPOSITE(g/257, alpha, chb[1], 1);
				chb[2] = MCOMPOSITE(b/257, alpha, chb[2], 1);
			}

			// Cloud overlay on channel A
			if(MCIR){
				float cloud = (chb[0] - 110) / 120;
				cha[0] = MCOMPOSITE(240, cloud, cha[0], 1);
				cha[1] = MCOMPOSITE(250, cloud, cha[1], 1);
				cha[2] = MCOMPOSITE(255, cloud, cha[2], 1);
			}
		}
	}

	return 1;
}

// Row where to satellite reaches peak elevation
int zenith = 0;

static int ImageOut(char *filename, char *chid, float **prow, int nrow, int width, int offset, char *palette, char *effects, char *mapFile) {
    FILE *pngfile;

	// Reduce the width of the image to componsate for the missing telemetry
	if(CONTAINS(effects, 't')) width -= TOTAL_TELE;

	// Create writer
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
		png_destroy_write_struct(&png_ptr, (png_infopp) NULL);
		fprintf(stderr, ERR_PNG_WRITE);
		return(0);
    }
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
		png_destroy_write_struct(&png_ptr, (png_infopp) NULL);
		fprintf(stderr, ERR_PNG_INFO);
		return(0);
    }

	// 8 bit RGB image
	png_set_IHDR(png_ptr, info_ptr, width, nrow,
	 		 	 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
	 			 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    text_ptr[1].text = chid;
    text_ptr[1].text_length = strlen(chid);
    png_set_text(png_ptr, info_ptr, text_ptr, 3);
    png_set_pHYs(png_ptr, info_ptr, 4160, 4160, PNG_RESOLUTION_METER);

	// Init I/O
    pngfile = fopen(filename, "wb");
    if (!pngfile) {
		fprintf(stderr, ERR_FILE_WRITE, filename);
		return(1);
    }
    png_init_io(png_ptr, pngfile);
    png_write_info(png_ptr, info_ptr);

	// Move prow into crow, crow ~ color rows
	float *crow[3000];
	for(int i = 0; i < nrow; i++){
		crow[i] = (float *) malloc(sizeof(float) * 2080 * 3);

		for(int x = 0; x < 2080; x++)
			crow[i][x*3 + 0] = crow[i][x*3 + 1] = crow[i][x*3 + 2] = prow[i][x];
	}

	if(mapFile != NULL && mapFile[0] != '\0'){
		if(mapOverlay(mapFile, crow, nrow, zenith, strcmp(chid, "MCIR") == 0) == 0){
			fprintf(stderr, "Skipping MCIR generation; see above.\n");
			return 0;
		}
	}else if(strcmp(chid, "MCIR")){
		fprintf(stderr, "Skipping MCIR generation; no map provided.\n");
		return 0;
	}

	printf("Writing %s", filename);

	// Build RGB image
    for (int n = 0; n < nrow; n++) {
		png_color pix[width];

		for (int i = 0; i < width; i++) {
			float *px = &crow[n][offset*3 + i*3];
			pix[i].red   = CLIP(px[0], 0, 255);
			pix[i].green = CLIP(px[1], 0, 255);
			pix[i].blue  = CLIP(px[2], 0, 255);
		}
		
		png_write_row(png_ptr, (png_bytep) pix);
    }

	// Tidy up
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

	ImageOut(filename, "Brightness distribution", distrib, 256, 256, 0, NULL, 0, NULL);
}

extern int calibrate(float **prow, int nrow, int offset, int width);
extern void histogramEqualise(float **prow, int nrow, int offset, int width);
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
	" -e [t|h]       Enhancements\n"
	"     t: Crop telemetry\n"
	"     h: Histogram equalise\n"
	" -i [r|a|b|c|t] Output image type\n"
	"     r: Raw\n"
	"     a: Channel A\n"
	"     b: Channel B\n"
	"     c: False color\n"
	"     t: Temperature\n"
	"     m: MCIR\n"
	" -d <dir>       Image destination directory.\n"
	" -s [15-19]     Satellite number\n"
	" -c <file>      False color config file\n"
	" -m <file>      Map file\n");
   	exit(1);
}

int readRawImage(char *filename, float **prow, int *nrow) {
	FILE *fp = fopen(filename, "r");

	// Create reader
	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(!png) return 0;
	png_infop info = png_create_info_struct(png);
	if(!info) return 0;
	png_init_io(png, fp);

	// Read info from header
	png_read_info(png, info);
	int width = png_get_image_width(png, info);
	int height = png_get_image_height(png, info);
	png_byte color_type = png_get_color_type(png, info);
	png_byte bit_depth  = png_get_bit_depth(png, info);

	// Check the image
	if(width != 2080){
		fprintf(stderr, "Raw image must be 2080px wide.\n");
		return 0;
	}else if(bit_depth != 8){
		fprintf(stderr, "Raw image must have 8 bit color.\n");
		return 0;
	}else if(color_type != PNG_COLOR_TYPE_GRAY){
		fprintf(stderr, "Raw image must be grayscale.\n");
		return 0;
	}

	// Create row buffers
	png_bytep *PNGrows = NULL;
	PNGrows = (png_bytep *) malloc(sizeof(png_bytep) * height);
	for(int y = 0; y < height; y++) PNGrows[y] = (png_byte *) malloc(png_get_rowbytes(png, info));

	// Read image
	png_read_image(png, PNGrows);

	// Tidy up
	fclose(fp);
	png_destroy_read_struct(&png, &info, NULL);

	// Put into prow
	*nrow = height;
	for(int y = 0; y < height; y++) {
		prow[y] = (float *) malloc(sizeof(float) * width);

		for(int x = 0; x < width; x++)
			prow[y][x] = (float)PNGrows[y][x];
	}

	return 1;
}

int main(int argc, char **argv) {
    char pngfilename[1024];
    char name[128];
    char pngdirname[128] = "";
	char mapFile[256];
	char *extension;

	// Default to a raw image, with no enhancements
    char imgopt[20] = "r";
	char enhancements[20] = "";

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
    while ((c = getopt(argc, argv, "c:m:d:i:s:e:")) != EOF) {
		switch (c) {
			// Output directory name
			case 'd':
				strcpy(pngdirname, optarg);
				break;
			// False color config file
			case 'c':
				readfcconf(optarg);
				break;
			// Map file
			case 'm':
				strcpy(mapFile, optarg);
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
				strncpy(enhancements, optarg, 20);
				break;
			default:
				usage();
		}
    }
	if(optind == argc){
		printf("No input files provided.\n");
		usage();
	}

	// Process the provided files
	for (; optind < argc; optind++) {
		chA = chB = 0;

		// Generate output name
		strcpy(pngfilename, argv[optind]);
		strcpy(name, basename(pngfilename));
		strtok(name, ".");
		extension = strtok(NULL, ".");

		if (pngdirname[0] == '\0')
			strcpy(pngdirname, dirname(pngfilename));

		if(strcmp(extension, "png") == 0){
			printf("Reading %s", argv[optind]);
			if(readRawImage(argv[optind], prow, &nrow) == 0){
				fprintf(stderr, "Skipping %s; see above.\n", name);
				continue;
			}
		}else{
			// Open sound file, exit if that fails
			if (initsnd(argv[optind]) == 0) exit(1);

			// Main image building loop
			for (nrow = 0; nrow < 3000; nrow++) {
				// Allocate 2150 floats worth of memory for every line of the image
				prow[nrow] = (float *) malloc(sizeof(float) * 2150);
				
				// Read into prow and break the loop once we reach the end of the image
				if (getpixelrow(prow[nrow], nrow, &zenith) == 0) break;

				printf("Row: %d\r", nrow);
				fflush(stdout);
			}

			// Close sound file
			sf_close(inwav);
		}

		if(zenith == 0 & mapFile[0] != '\0'){
			fprintf(stderr, "WARNING: Guessing peak elevation in image, map will most likely not be aligned.\n");
			zenith = nrow / 2;
		}

		printf("\nTotal rows: %d\n", nrow);

		// Calibrate
		chA = calibrate(prow, nrow, CHA_OFFSET, CH_WIDTH);
		chB = calibrate(prow, nrow, CHB_OFFSET, CH_WIDTH);
		printf("Channel A: %s (%s)\n", ch.id[chA], ch.name[chA]);
		printf("Channel B: %s (%s)\n", ch.id[chB], ch.name[chB]);

		// Temperature
		if (CONTAINS(imgopt, 't') && chB >= 4) {
			// TODO: Doesn't work with channel 4
			temperature(prow, nrow, chB, CHB_OFFSET);
			sprintf(pngfilename, "%s/%s-t.png", pngdirname, name);
			ImageOut(pngfilename, "Temperature", prow, nrow, 2080, 0, TempPalette, enhancements, mapFile);
		}

		// False color image
		if(CONTAINS(imgopt, 'c')){
			if (chA == 2 && chB >= 4) { // Normal false color
				sprintf(pngfilename, "%s/%s-c.png", pngdirname, name);
				//ImageRGBOut(pngfilename, prow, nrow);
				ImageOut(pngfilename, "False Color", prow, nrow, CH_WIDTH, 0, NULL, enhancements, mapFile);
			} else if (chB == 2) { // GVI (global vegetation index) false color
				Ngvi(prow, nrow);
				sprintf(pngfilename, "%s/%s-c.png", pngdirname, name);
				ImageOut(pngfilename, "GVI False Color", prow, nrow, CH_WIDTH, CHB_OFFSET, GviPalette, enhancements, mapFile);
			} else {
				fprintf(stderr, "Skipping False Color generation; lacking required channels.\n");
			}
		}

		// MCIR
		if (CONTAINS(imgopt, 'm')) {
			sprintf(pngfilename, "%s/%s-m.png", pngdirname, name);
			ImageOut(pngfilename, "MCIR", prow, nrow, 909, CHA_OFFSET, NULL, enhancements, mapFile);
		}

		// Histogram equalise
		if(CONTAINS(enhancements, 'h')){
			histogramEqualise(prow, nrow, CHA_OFFSET, CH_WIDTH);
			histogramEqualise(prow, nrow, CHB_OFFSET, CH_WIDTH);
		}

		// Raw image
		if (CONTAINS(imgopt, 'r')) {
			char channelstr[45];
			sprintf(channelstr, "%s (%s) & %s (%s)", ch.id[chA], ch.name[chA], ch.id[chB], ch.name[chB]);

			sprintf(pngfilename, "%s/%s-r.png", pngdirname, name);
			ImageOut(pngfilename, channelstr, prow, nrow, IMG_WIDTH, 0, NULL, enhancements, mapFile);
		}

		// Channel A
		if (CONTAINS(imgopt, 'a')) {
			char channelstr[21];
			sprintf(channelstr, "%s (%s)", ch.id[chA], ch.name[chA]);

			sprintf(pngfilename, "%s/%s-%s.png", pngdirname, name, ch.id[chA]);
			ImageOut(pngfilename, channelstr, prow, nrow, CH_WIDTH, CHA_OFFSET, NULL, enhancements, mapFile);
		}

		// Channel B
		if (CONTAINS(imgopt, 'b')) {
			char channelstr[21];
			sprintf(channelstr, "%s (%s)", ch.id[chB], ch.name[chB]);

			sprintf(pngfilename, "%s/%s-%s.png", pngdirname, name, ch.id[chB]);
			ImageOut(pngfilename, channelstr, prow, nrow, CH_WIDTH , CHB_OFFSET, NULL, enhancements, mapFile);
		}

		// Distribution image
		if (CONTAINS(imgopt, 'd')) {
			sprintf(pngfilename, "%s/%s-d.png", pngdirname, name);
			distrib(pngfilename, prow, nrow);
		}
	}

    exit(0);
}
