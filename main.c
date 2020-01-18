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
#include <getopt.h>
#include <libgen.h>
#include <math.h>
#include <sndfile.h>
#include <errno.h>

#include "messages.h"
#include "offsets.h"

#define FAILURE 0
#define SUCCESS 1

typedef struct {
	char *type; // Output image type
	char *effects;
	int   satnum; // The satellite number
	char *map; // Path to a map file
	char *path; // Output directory
	int   realtime;
} options_t;

typedef struct {
	float *prow[MAX_HEIGHT]; // Row buffers
	int nrow; // Number of rows
	int chA, chB; // ID of each channel
	char name[256]; // Stripped filename
} image_t;

// DSP
extern int getpixelrow(float *pixelv, int nrow, int *zenith);
extern int init_dsp(double F);

// I/O
extern int readfcconf(char *file);
extern int readRawImage(char *filename, float **prow, int *nrow);
extern int ImageOut(options_t *opts, image_t *img, int offset, int width, char *desc, char *chid, char *palette);
extern void closeWriter();
extern void pushRow(float *row, int width);
extern int initWriter(options_t *opts, image_t *img, int width, int height, char *desc, char *chid);

// Image functions
extern int calibrate(float **prow, int nrow, int offset, int width);
extern void histogramEqualise(float **prow, int nrow, int offset, int width);
extern void temperature(options_t *opts, image_t *img, int offset, int width);
extern int Ngvi(float **prow, int nrow);
extern void denoise(float **prow, int nrow, int offset, int width);
extern void distrib(options_t *opts, image_t *img, char *chid);

// Palettes
extern char GviPalette[256*3];
extern char TempPalette[256*3];

// Row where the satellite is closest to the observer
int zenith = 0;
// Audio file
static SNDFILE *audioFile;

static int initsnd(char *filename);
int getsample(float *sample, int nb);
static int processAudio(char *filename, options_t *opts);
static void usage(void);

int main(int argc, char **argv) {
    fprintf(stderr, VERSION"\n");

	if(argc == 1)
		usage();

	// Check if there are actually any input files
	if(optind == argc){
		fprintf(stderr, "No input files provided.\n");
		usage();
	}

	options_t opts = { "r", "", 19, "", ".", 0 };

	// Parse arguments
	int opt;
    while ((opt = getopt(argc, argv, "c:m:d:i:s:e:r")) != EOF) {
		switch (opt) {
			case 'd':
				opts.path = optarg;
				break;
			case 'c':
				readfcconf(optarg);
				break;
			case 'm':
				opts.map = optarg;
				break;
			case 'i':
				opts.type = optarg;
				break;
			case 's':
				opts.satnum = atoi(optarg);
				if(opts.satnum < 15 || opts.satnum > 19){
					fprintf(stderr, "Invalid satellite number, it must be the range 15-19\n");
					exit(EPERM);
				}
				break;
			case 'e':
				opts.effects = optarg;
				break;
			case 'r':
				opts.realtime = 1;
				break;
			default:
				usage();
		}
    }

	// Process the files
	for (; optind < argc; optind++) {
		processAudio(argv[optind], &opts);
	}

    exit(0);
}

static int processAudio(char *filename, options_t *opts){
	// Image info struct
	image_t img;

	// Mapping between wedge value and channel ID
	static struct {
		char *id[7];
		char *name[7];
	} ch = {
		{ "?", 		 "1", 	   "2", 			"3A", 		    "4", 				"5", 				"3B"           },
		{ "unknown", "visble", "near-infrared", "mid-infrared", "thermal-infrared", "thermal-infrared", "mid-infrared" }
	};

	// Buffer for image channel
	char desc[60];

	// Parse file path
	char path[256], extension[32];
	strcpy(path, filename);
	strcpy(path, dirname(path));
	sscanf(basename(filename), "%[^.].%s", img.name, extension);

	if(opts->realtime) initWriter(opts, &img, IMG_WIDTH, MAX_HEIGHT, "Unprocessed realtime image", "r");

	if(strcmp(extension, "png") == 0){
		// Read PNG into image buffer
		printf("Reading %s", filename);
		if(readRawImage(filename, img.prow, &img.nrow) == 0){
			fprintf(stderr, "Skipping %s; see above.\n", img.name);
			return FAILURE;
		}
	}else{
		// Attempt to open the audio file
		if (initsnd(filename) == 0)
			exit(EPERM);

		// Build image
		for (img.nrow = 0; img.nrow < MAX_HEIGHT; img.nrow++) {
			// Allocate memory for this row
			img.prow[img.nrow] = (float *) malloc(sizeof(float) * 2150);
				
			// Write into memory and break the loop when there are no more samples to read
			if (getpixelrow(img.prow[img.nrow], img.nrow, &zenith) == 0)
				break;

			if(opts->realtime) pushRow(img.prow[img.nrow], IMG_WIDTH);

			fprintf(stderr, "Row: %d\r", img.nrow);
			fflush(stderr);
		}

		// Close stream
		sf_close(audioFile);
	}

	if(opts->realtime) closeWriter();

	printf("\nTotal rows: %d\n", img.nrow);

	// Fallback for detecting the zenith
	// TODO: encode zenith in raw images
	if(opts->map != NULL && opts->map[0] != '\0' && zenith == 0){
		fprintf(stderr, "Guessing zenith in image, map will most likely be misaligned.\n");
		zenith = img.nrow / 2;
	}

	// Calibrate
	img.chA = calibrate(img.prow, img.nrow, CHA_OFFSET, CH_WIDTH);
	img.chB = calibrate(img.prow, img.nrow, CHB_OFFSET, CH_WIDTH);
	printf("Channel A: %s (%s)\n", ch.id[img.chA], ch.name[img.chA]);
	printf("Channel B: %s (%s)\n", ch.id[img.chB], ch.name[img.chB]);

	// Denoise
	if(CONTAINS(opts->effects, 'd')){
		denoise(img.prow, img.nrow, CHA_OFFSET, CH_WIDTH);
		denoise(img.prow, img.nrow, CHB_OFFSET, CH_WIDTH);
	}

	// Temperature
	if (CONTAINS(opts->type, 't') && img.chB >= 4) {
		temperature(opts, &img, CHB_OFFSET, CH_WIDTH);
		ImageOut(opts, &img, CHB_OFFSET, CH_WIDTH, "Temperature", "t", (char *)TempPalette);
	}

	// False color image
	if(CONTAINS(opts->type, 'c')){
		if (img.chA == 2 && img.chB >= 4) { // Normal false color
			// TODO: use real MSA
			// TODO: provide more than just "natural" color images
			ImageOut(opts, &img, 0, CH_WIDTH, "False Color", "c", NULL);
		} else if (img.chB == 2) { // GVI (global vegetation index) false color
			Ngvi(img.prow, img.nrow);
			ImageOut(opts, &img, CHB_OFFSET, CH_WIDTH, "GVI False Color", "c", (char *)GviPalette);
		} else {
			fprintf(stderr, "Skipping False Color generation; lacking required channels.\n");
		}
	}

	// MCIR
	if (CONTAINS(opts->type, 'm'))
		ImageOut(opts, &img, 0, IMG_WIDTH, "MCIR", "m", NULL);

	// Histogram equalise
	if(CONTAINS(opts->effects, 'h')){
		histogramEqualise(img.prow, img.nrow, CHA_OFFSET, CH_WIDTH);
		histogramEqualise(img.prow, img.nrow, CHB_OFFSET, CH_WIDTH);
	}

	// Raw image
	if (CONTAINS(opts->type, 'r')) {
		sprintf(desc, "%s (%s) & %s (%s)", ch.id[img.chA], ch.name[img.chA], ch.id[img.chB], ch.name[img.chB]);
		ImageOut(opts, &img, 0, IMG_WIDTH, desc, "r", NULL);
	}

	// Channel A
	if (CONTAINS(opts->type, 'a')) {
		sprintf(desc, "%s (%s)", ch.id[img.chA], ch.name[img.chA]);
		ImageOut(opts, &img, CHA_OFFSET, CH_WIDTH, desc, ch.id[img.chA], NULL);
	}

	// Channel B
	if (CONTAINS(opts->type, 'b')) {
		sprintf(desc, "%s (%s)", ch.id[img.chB], ch.name[img.chB]);
		ImageOut(opts, &img, CHB_OFFSET, CH_WIDTH, desc, ch.id[img.chB], NULL);
	}

	// Distribution image
	if (CONTAINS(opts->type, 'd'))
		distrib(opts, &img, "d");
	
	return SUCCESS;
}

static int initsnd(char *filename) {
    SF_INFO infwav;
    int	res;

	// Open audio file
    infwav.format = 0;
    audioFile = sf_open(filename, SFM_READ, &infwav);
    if (audioFile == NULL) {
		fprintf(stderr, ERR_FILE_READ, filename);
		return FAILURE;
    }

    res = init_dsp(infwav.samplerate);
	printf("Input file: %s\n", filename);
    if(res < 0) {
		fprintf(stderr, "Input sample rate too low: %d\n", infwav.samplerate);
		return FAILURE;
    }else if(res > 0) {
		fprintf(stderr, "Input sample rate too high: %d\n", infwav.samplerate);
		return FAILURE;
    }
    printf("Input sample rate: %d\n", infwav.samplerate);

	// TODO: accept stereo audio
    if (infwav.channels != 1) {
		fprintf(stderr, "Too many channels in input file: %d\n", infwav.channels);
		return FAILURE;
    }

    return SUCCESS;
}

// Read samples from the wave file
int getsample(float *sample, int nb) {
	return sf_read_float(audioFile, sample, nb);
}

static void usage(void) {
    fprintf(stderr,
	"Aptdec [options] audio files ...\n"
	"Options:\n"
	" -e [t|h]       Effects\n"
	"     t: Crop telemetry\n"
	"     h: Histogram equalise\n"
	"     d: Denoise\n"
	"     p: Precipitation\n"
	" -i [r|a|b|c|t|m] Output image\n"
	"     r: Raw\n"
	"     a: Channel A\n"
	"     b: Channel B\n"
	"     c: False color\n"
	"     t: Temperature\n"
	"     m: MCIR\n"
	" -d <dir>       Image destination directory.\n"
	" -s [15-19]     Satellite number\n"
	" -c <file>      False color config file\n"
	" -m <file>      Map file\n"
	" -r             Realtime decode\n");

   	exit(EINVAL);
}
