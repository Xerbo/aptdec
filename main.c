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
#include <time.h>

#include "common.h"
#include "offsets.h"

// DSP
extern int init_dsp(double F);
extern int getpixelrow(float *pixelv, int nrow, int *zenith, int reset);

// I/O
extern int readRawImage(char *filename, float **prow, int *nrow);
extern int ImageOut(options_t *opts, image_t *img, int offset, int width, char *desc, char *chid, char *palette);
extern int initWriter(options_t *opts, image_t *img, int width, int height, char *desc, char *chid);
extern void pushRow(float *row, int width);
extern void closeWriter();

// Image functions
extern int calibrate(float **prow, int nrow, int offset, int width);
extern void histogramEqualise(float **prow, int nrow, int offset, int width);
extern void linearEnhance(float **prow, int nrow, int offset, int width);
extern void temperature(options_t *opts, image_t *img, int offset, int width);
extern void denoise(float **prow, int nrow, int offset, int width);
extern void distrib(options_t *opts, image_t *img, char *chid);
extern void flipImage(image_t *img, int width, int offset);

// Palettes
extern char GviPalette[256*3];
extern char TempPalette[256*3];

// Row where the satellite is closest to the observer
int zenith = 0;
// Audio file
static SNDFILE *audioFile;
// Number of channels in audio file
int channels = 1;

// Function declarations
static int initsnd(char *filename);
int getsample(float *sample, int nb);
static int processAudio(char *filename, options_t *opts);
static void usage(void);

int main(int argc, char **argv) {
	fprintf(stderr, VERSION"\n");

	// Check if there are actually any input files
	if(argc == optind || argc == 1){
		fprintf(stderr, "No input files provided.\n");
		usage();
	}

	options_t opts = { "r", "", 19, "", ".", 0, "" };

	// Parse arguments
	int opt;
	while ((opt = getopt(argc, argv, "o:m:d:i:s:e:rp:")) != EOF) {
		switch (opt) {
			case 'd':
				opts.path = optarg;
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
			case 'o':
				opts.filename = optarg;
				break;
			case 'p':
				opts.palette = optarg;
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
		{ "?", 		 "1", 	   "2", 			"3A", 			"4", 				"5", 				"3B"		   },
		{ "unknown", "visble", "near-infrared", "mid-infrared", "thermal-infrared", "thermal-infrared", "mid-infrared" }
	};

	// Buffer for image channel
	char desc[60];

	// Parse file path
	char path[256], extension[32];
	strcpy(path, filename);
	strcpy(path, dirname(path));
	sscanf(basename(filename), "%[^.].%s", img.name, extension);

	if(opts->realtime){
		// Set output filename to current time when in realtime mode
		time_t t;
		time(&t);
		strncpy(img.name, ctime(&t), 24);

		// Init a row writer
		initWriter(opts, &img, IMG_WIDTH, MAX_HEIGHT, "Unprocessed realtime image", "r");
	}		

	if(strcmp(extension, "png") == 0){
		// Read PNG into image buffer
		printf("Reading %s\n", filename);
		if(readRawImage(filename, img.prow, &img.nrow) == 0){
			fprintf(stderr, "Skipping %s\n", img.name);
			return 0;
		}
	}else{
		// Attempt to open the audio file
		if (initsnd(filename) == 0)
			exit(EPERM);

		// Build image
		// TODO: multithreading, would require some sort of input buffer
		for (img.nrow = 0; img.nrow < MAX_HEIGHT; img.nrow++) {
			// Allocate memory for this row
			img.prow[img.nrow] = (float *) malloc(sizeof(float) * 2150);

			// Write into memory and break the loop when there are no more samples to read
			if (getpixelrow(img.prow[img.nrow], img.nrow, &zenith, (img.nrow == 0)) == 0)
				break;

			if(opts->realtime) pushRow(img.prow[img.nrow], IMG_WIDTH);

			fprintf(stderr, "Row: %d\r", img.nrow);
			fflush(stderr);
		}

		// Close stream
		sf_close(audioFile);
	}

	if(opts->realtime) closeWriter();

	printf("Total rows: %d\n", img.nrow);

	// Fallback for detecting the zenith
	// TODO: encode metadata in raw images
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

	// Flip, for southbound passes
	if(CONTAINS(opts->effects, 'f')){
		flipImage(&img, CH_WIDTH, CHA_OFFSET);
		flipImage(&img, CH_WIDTH, CHB_OFFSET);
	}

	// Temperature
	if (CONTAINS(opts->type, 't') && img.chB >= 4) {
		temperature(opts, &img, CHB_OFFSET, CH_WIDTH);
		ImageOut(opts, &img, CHB_OFFSET, CH_WIDTH, "Temperature", "t", (char *)TempPalette);
	}

	// MCIR
	if (CONTAINS(opts->type, 'm'))
		ImageOut(opts, &img, CHA_OFFSET, CH_WIDTH, "MCIR", "m", NULL);

	// Linear equalise
	if(CONTAINS(opts->effects, 'l')){
		linearEnhance(img.prow, img.nrow, CHA_OFFSET, CH_WIDTH);
		linearEnhance(img.prow, img.nrow, CHB_OFFSET, CH_WIDTH);
	}

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

	// Palette image
	if (CONTAINS(opts->type, 'p')) {
		img.palette = opts->palette;
		strcpy(desc, "Palette composite");
		ImageOut(opts, &img, 0, 909, desc, "p", NULL);
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

	// Value distribution image
	if (CONTAINS(opts->type, 'd'))
		distrib(opts, &img, "d");

	return 1;
}

static int initsnd(char *filename) {
	SF_INFO infwav;
	int	res;

	// Open audio file
	infwav.format = 0;
	audioFile = sf_open(filename, SFM_READ, &infwav);
	if (audioFile == NULL) {
		fprintf(stderr, "Could not open %s for reading\n", filename);
		return 0;
	}

	res = init_dsp(infwav.samplerate);
	printf("Input file: %s\n", filename);
	if(res < 0) {
		fprintf(stderr, "Input sample rate too low: %d\n", infwav.samplerate);
		return 0;
	}else if(res > 0) {
		fprintf(stderr, "Input sample rate too high: %d\n", infwav.samplerate);
		return 0;
	}
	printf("Input sample rate: %d\n", infwav.samplerate);

	channels = infwav.channels;

	return 1;
}

// Read samples from the wave file
int getsample(float *sample, int nb) {
	if(channels == 1){
		return sf_read_float(audioFile, sample, nb);
	}else{
		/* Multi channel audio is encoded such as:
		 *  Ch1,Ch2,Ch1,Ch2,Ch1,Ch2
		 */
		float buf[nb * channels]; // Something like BLKIN*2 could also be used
		int samples = sf_read_float(audioFile, buf, nb * channels);
		for(int i = 0; i < nb; i++) sample[i] = buf[i * channels];
		return samples / channels;
	}
}

static void usage(void) {
	fprintf(stderr,
	"Aptdec [options] audio files ...\n"
	"Options:\n"
	" -e [t|h|d|p|f|l] Effects\n"
	"    t: Crop telemetry\n"
	"    h: Histogram equalise\n"
	"    d: Denoise\n"
	"    p: Precipitation\n"
	"    f: Flip image\n"
	"    l: Linear equalise\n"
	" -i [r|a|b|c|t|m|p] Output image\n"
	"    r: Raw\n"
	"    a: Channel A\n"
	"    b: Channel B\n"
	"    t: Temperature\n"
	"    m: MCIR\n"
	"    p: Paletted image\n"
	" -d <dir>         Image destination directory.\n"
	" -o <name>        Output filename\n"
	" -s [15-19]       Satellite number\n"
	" -m <file>        Map file\n"
	" -r               Realtime decode\n"
	" -p               Path to palette\n"
	"\nRefer to the README for more infomation\n");

	exit(EINVAL);
}
