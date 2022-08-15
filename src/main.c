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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifndef _MSC_VER
#include <libgen.h>
#else
#include <windows.h>
#endif
#include <math.h>
#include <sndfile.h>
#include <errno.h>
#include <time.h>
#include "argparse/argparse.h"

#include "common.h"
#include "apt.h"

#include "pngio.h"
#include "image.h"
#include "color.h"
#include "util.h"

// Audio file
static SNDFILE *audioFile;
// Number of channels in audio file
int channels = 1;

// Function declarations
static int initsnd(char *filename);
int getsamples(void *context, float *samples, int nb);
static int processAudio(char *filename, options_t *opts);

#ifdef _MSC_VER
// Functions not supported by MSVC
static char *dirname(char *path)
{
	static char dir[MAX_PATH];
	_splitpath(path, NULL, dir, NULL, NULL);
        return dir;
}

static char *basename(char *path)
{
	static char base[MAX_PATH];
	_splitpath(path, NULL, NULL, base, NULL);
        return base;
}
#endif

int main(int argc, const char **argv) {
	options_t opts = { 
		.type = "r",
		.effects = "",
		.satnum = 19,
		.path = ".",
		.realtime = 0,
		.filename = "",
		.palette = "",
		.gamma = 1.0
	};

	static const char *const usages[] = {
		"aptdec [options] [[--] sources]",
		"aptdec [sources]",
		NULL,
	};

	struct argparse_option options[] = {
		OPT_HELP(),
		OPT_GROUP("Image options"),
		OPT_STRING('i', "image", &opts.type, "set output image type (see the README for a list)", NULL, 0, 0),
		OPT_STRING('e', "effect", &opts.effects, "add an effect (see the README for a list)", NULL, 0, 0),
		OPT_FLOAT('g', "gamma", &opts.gamma, "gamma adjustment (1.0 = off)", NULL, 0, 0),

		OPT_GROUP("Satellite options"),
		OPT_INTEGER('s', "satellite", &opts.satnum, "satellite ID, must be between 15 and 19", NULL, 0, 0),

		OPT_GROUP("Paths"),
		OPT_STRING('p', "palette", &opts.palette, "path to a palette", NULL, 0, 0),
		OPT_STRING('o', "filename", &opts.filename, "filename of the output image", NULL, 0, 0),
		OPT_STRING('d', "output", &opts.path, "output directory (must exist first)", NULL, 0, 0),

		OPT_GROUP("Misc"),
		OPT_BOOLEAN('r', "realtime", &opts.realtime, "decode in realtime", NULL, 0, 0),
		OPT_END(),
	};

	struct argparse argparse;
	argparse_init(&argparse, options, usages, 0);
	argparse_describe(&argparse, "\nA lightweight FOSS NOAA APT satellite imagery decoder.", "\nSee `README.md` for a full description of command line arguments and `LICENSE` for licensing conditions.");
	argc = argparse_parse(&argparse, argc, argv);

	if(argc == 0){
		argparse_usage(&argparse);
	}

	// Actually decode the files
	for (int i = 0; i < argc; i++) {
		char *filename = strdup(argv[i]);
		processAudio(filename, &opts);
	}

	return 0;
}

static int processAudio(char *filename, options_t *opts){
	// Image info struct
	apt_image_t img;

	// Mapping between wedge value and channel ID
	static struct {
		char *id[7];
		char *name[7];
	} ch = {
		{ "?", 		 "1", 	   "2", 			"3A", 			"4", 				"5", 				"3B"		   },
		{ "unknown", "visble", "near-infrared", "near-infrared", "thermal-infrared", "thermal-infrared", "mid-infrared" }
	};

	// Buffer for image channel
	char desc[60];

	// Parse file path
	char path[256], extension[32];
	strcpy(path, filename);
	strcpy(path, dirname(path));
	sscanf(basename(filename), "%255[^.].%31s", img.name, extension);

	if(opts->realtime){
		// Set output filename to current time when in realtime mode
		time_t t;
		time(&t);
		strncpy(img.name, ctime(&t), 24);

		// Init a row writer
		initWriter(opts, &img, APT_IMG_WIDTH, APT_MAX_HEIGHT, "Unprocessed realtime image", "r");
	}		

	if(strcmp(extension, "png") == 0){
		// Read PNG into image buffer
		printf("Reading %s\n", filename);
		if(readRawImage(filename, img.prow, &img.nrow) == 0){
			exit(EPERM);
		}
	}else{
		// Attempt to open the audio file
		if (initsnd(filename) == 0)
			exit(EPERM);

		// Build image
		// TODO: multithreading, would require some sort of input buffer
		for (img.nrow = 0; img.nrow < APT_MAX_HEIGHT; img.nrow++) {
			// Allocate memory for this row
			img.prow[img.nrow] = (float *) malloc(sizeof(float) * APT_PROW_WIDTH);

			// Write into memory and break the loop when there are no more samples to read
			if (apt_getpixelrow(img.prow[img.nrow], img.nrow, &img.zenith, (img.nrow == 0), getsamples, NULL) == 0)
				break;

			if(opts->realtime) pushRow(img.prow[img.nrow], APT_IMG_WIDTH);

			fprintf(stderr, "Row: %d\r", img.nrow);
			fflush(stderr);
		}

		// Close stream
		sf_close(audioFile);
	}

	if(opts->realtime) closeWriter();

	printf("Total rows: %d\n", img.nrow);

	// Calibrate
	img.chA = apt_calibrate(img.prow, img.nrow, APT_CHA_OFFSET, APT_CH_WIDTH);
	img.chB = apt_calibrate(img.prow, img.nrow, APT_CHB_OFFSET, APT_CH_WIDTH);
	printf("Channel A: %s (%s)\n", ch.id[img.chA], ch.name[img.chA]);
	printf("Channel B: %s (%s)\n", ch.id[img.chB], ch.name[img.chB]);

	// Crop noise from start and end of image
	if(CONTAINS(opts->effects, Crop_Noise)){
		img.zenith -= apt_cropNoise(&img);
	}

	// Denoise
	if(CONTAINS(opts->effects, Denoise)){
		apt_denoise(img.prow, img.nrow, APT_CHA_OFFSET, APT_CH_WIDTH);
		apt_denoise(img.prow, img.nrow, APT_CHB_OFFSET, APT_CH_WIDTH);
	}

	// Flip, for northbound passes
	if(CONTAINS(opts->effects, Flip_Image)){
		apt_flipImage(&img, APT_CH_WIDTH, APT_CHA_OFFSET);
		apt_flipImage(&img, APT_CH_WIDTH, APT_CHB_OFFSET);
	}

	// Temperature
	if (CONTAINS(opts->type, Temperature) && img.chB >= 4) {
		// Create another buffer as to not modify the orignal
		apt_image_t tmpimg = img;
		for(int i = 0; i < img.nrow; i++){
			tmpimg.prow[i] = (float *) malloc(sizeof(float) * APT_PROW_WIDTH);
			memcpy(tmpimg.prow[i], img.prow[i], sizeof(float) * APT_PROW_WIDTH);
		}

		// Perform temperature calibration
		apt_calibrate_thermal(opts->satnum, &tmpimg, APT_CHB_OFFSET, APT_CH_WIDTH);
		ImageOut(opts, &tmpimg, APT_CHB_OFFSET, APT_CH_WIDTH, "Temperature", Temperature, (char *)apt_TempPalette);
	}

	// Temperature
	if (CONTAINS(opts->type, Visible) && img.chA <= 2) {
		// Create another buffer as to not modify the orignal
		apt_image_t tmpimg = img;
		for(int i = 0; i < img.nrow; i++){
			tmpimg.prow[i] = (float *) malloc(sizeof(float) * APT_PROW_WIDTH);
			memcpy(tmpimg.prow[i], img.prow[i], sizeof(float) * APT_PROW_WIDTH);
		}

		// Perform visible calibration
		apt_calibrate_visible(opts->satnum, &tmpimg, APT_CHA_OFFSET, APT_CH_WIDTH);
		ImageOut(opts, &tmpimg, APT_CHA_OFFSET, APT_CH_WIDTH, "Visible", Visible, (char *)apt_TempPalette);
	}

	// Linear equalise
	if(CONTAINS(opts->effects, Linear_Equalise)){
		apt_linearEnhance(img.prow, img.nrow, APT_CHA_OFFSET, APT_CH_WIDTH);
		apt_linearEnhance(img.prow, img.nrow, APT_CHB_OFFSET, APT_CH_WIDTH);
	}

	// Histogram equalise
	if(CONTAINS(opts->effects, Histogram_Equalise)){
		apt_histogramEqualise(img.prow, img.nrow, APT_CHA_OFFSET, APT_CH_WIDTH);
		apt_histogramEqualise(img.prow, img.nrow, APT_CHB_OFFSET, APT_CH_WIDTH);
	}

	// Raw image
	if (CONTAINS(opts->type, Raw_Image)) {
		sprintf(desc, "%s (%s) & %s (%s)", ch.id[img.chA], ch.name[img.chA], ch.id[img.chB], ch.name[img.chB]);
		ImageOut(opts, &img, 0, APT_IMG_WIDTH, desc, Raw_Image, NULL);
	}

	// Palette image
	if (CONTAINS(opts->type, Palleted)) {
		img.palette = opts->palette;
		strcpy(desc, "Palette composite");
		ImageOut(opts, &img, APT_CHA_OFFSET, APT_CH_WIDTH, desc, Palleted, NULL);
	}

	// Channel A
	if (CONTAINS(opts->type, Channel_A)) {
		sprintf(desc, "%s (%s)", ch.id[img.chA], ch.name[img.chA]);
		ImageOut(opts, &img, APT_CHA_OFFSET, APT_CH_WIDTH, desc, Channel_A, NULL);
	}

	// Channel B
	if (CONTAINS(opts->type, Channel_B)) {
		sprintf(desc, "%s (%s)", ch.id[img.chB], ch.name[img.chB]);
		ImageOut(opts, &img, APT_CHB_OFFSET, APT_CH_WIDTH, desc, Channel_B, NULL);
	}

	return 1;
}

float *samplebuf;
static int initsnd(char *filename) {
	SF_INFO infwav;
	int	res;

	// Open audio file
	infwav.format = 0;
	audioFile = sf_open(filename, SFM_READ, &infwav);
	if (audioFile == NULL) {
		error_noexit("Could not file");
		return 0;
	}

	res = apt_init(infwav.samplerate);
	printf("Input file: %s\n", filename);
	if(res < 0) {
		error_noexit("Input sample rate too low");
		return 0;
	}else if(res > 0) {
		error_noexit("Input sample rate too high");
		return 0;
	}
	printf("Input sample rate: %d\n", infwav.samplerate);

	channels = infwav.channels;
	samplebuf = (float *)malloc(sizeof(float) * 32768 * channels);

	return 1;
}

// Read samples from the audio file
int getsamples(void *context, float *samples, int nb) {
    (void) context;
	if (channels == 1){
		return (int)sf_read_float(audioFile, samples, nb);
	} else if (channels == 2) {
		// Stereo channels are interleaved
		int samplesRead = (int)sf_read_float(audioFile, samplebuf, nb * channels);
		for(int i = 0; i < nb; i++) {
			samples[i] = samplebuf[i * channels];
		}
		return samplesRead / channels;
	} else {
		printf("Only mono and stereo input files are supported\n");
		exit(1);
	}
}
