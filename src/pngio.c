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

#include <png.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include "util.h"

#include "pngio.h"

int readRawImage(char *filename, float **prow, int *nrow) {
	FILE *fp = fopen(filename, "rb");
    printf("%s", filename);
	if(!fp) {
		error_noexit("Cannot open image");
		return 0;
	}

	// Create reader
	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(!png) {
		fclose(fp);
		return 0;
	}
	png_infop info = png_create_info_struct(png);
	if(!info) {
		fclose(fp);
		return 0;
	}
	png_init_io(png, fp);

	// Read info from header
	png_read_info(png, info);
	int width = png_get_image_width(png, info);
	int height = png_get_image_height(png, info);
	png_byte color_type = png_get_color_type(png, info);
	png_byte bit_depth  = png_get_bit_depth(png, info);

	// Check the image
	if(width != APT_IMG_WIDTH){
		error_noexit("Raw image must be 2080px wide");
		return 0;
	}else if(bit_depth != 8){
		error_noexit("Raw image must have 8 bit color");
		return 0;
	}else if(color_type != PNG_COLOR_TYPE_GRAY){
		error_noexit("Raw image must be grayscale");
		return 0;
	}

	// Create row buffers
	png_bytep *PNGrows = NULL;
	PNGrows = (png_bytep *) malloc(sizeof(png_bytep) * height);
	for(int y = 0; y < height; y++) PNGrows[y] = (png_byte *)
		malloc(png_get_rowbytes(png, info));

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

int readPalette(char *filename, apt_rgb_t **pixels) {
	FILE *fp = fopen(filename, "rb");
	if(!fp) {
		error_noexit("Cannot open palette");
		return 0;
	}
	
	// Create reader
	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(!png) {
		fclose(fp);
		return 0;
	}
	png_infop info = png_create_info_struct(png);
	if(!info) {
		fclose(fp);
		return 0;
	}
	png_init_io(png, fp);

	// Read info from header
	png_read_info(png, info);
	int width = png_get_image_width(png, info);
	int height = png_get_image_height(png, info);
	png_byte color_type = png_get_color_type(png, info);
	png_byte bit_depth  = png_get_bit_depth(png, info);

	// Check the image
	if(width != 256 && height != 256){
		error_noexit("Palette must be 256x256");
		return 0;
	}else if(bit_depth != 8){
		error_noexit("Palette must be 8 bit color");
		return 0;
	}else if(color_type != PNG_COLOR_TYPE_RGB){
		error_noexit("Palette must be RGB");
		return 0;
	}

	// Create row buffers
	png_bytep *PNGrows = NULL;
	PNGrows = (png_bytep *) malloc(sizeof(png_bytep) * height);
	for(int y = 0; y < height; y++)
		PNGrows[y] = (png_byte *) malloc(png_get_rowbytes(png, info));

	// Read image
	png_read_image(png, PNGrows);

	// Tidy up
	fclose(fp);
	png_destroy_read_struct(&png, &info, NULL);

	// Put into crow
	for(int y = 0; y < height; y++) {
		pixels[y] = (apt_rgb_t *) malloc(sizeof(apt_rgb_t) * width);

		for(int x = 0; x < width; x++)
			pixels[y][x] = (apt_rgb_t){
				PNGrows[y][x*3],
				PNGrows[y][x*3 + 1], 
				PNGrows[y][x*3 + 2]
			};
	}

	return 1;
}

void prow2crow(float **prow, int nrow, char *palette, apt_rgb_t **crow){
	for(int y = 0; y < nrow; y++){
		crow[y] = (apt_rgb_t *) malloc(sizeof(apt_rgb_t) * APT_IMG_WIDTH);

		for(int x = 0; x < APT_IMG_WIDTH; x++){
			if(palette == NULL)
				crow[y][x].r = crow[y][x].g = crow[y][x].b = prow[y][x];
			else
				crow[y][x] = apt_applyPalette(palette, prow[y][x]);
		}
	}
}

int applyUserPalette(float **prow, int nrow, char *filename, apt_rgb_t **crow){
	apt_rgb_t *pal_row[256];
	if(!readPalette(filename, pal_row)){
		error_noexit("Could not read palette");
		return 0;
	}

	for(int y = 0; y < nrow; y++){
		for(int x = 0; x < APT_CH_WIDTH; x++){
			int cha = CLIP(prow[y][x + APT_CHA_OFFSET], 0, 255);
			int chb = CLIP(prow[y][x + APT_CHB_OFFSET], 0, 255);
			crow[y][x + APT_CHA_OFFSET] = pal_row[chb][cha];
		}
	}
	
	return 1;
}

int ImageOut(options_t *opts, apt_image_t *img, int offset, int width, char *desc, char chid, char *palette){
	char outName[512];
	if(opts->filename == NULL || opts->filename[0] == '\0'){
		sprintf(outName, "%s/%s-%c.png", opts->path, img->name, chid);
	}else{
		sprintf(outName, "%s/%s", opts->path, opts->filename);
	}

	png_text meta[] = {
		{PNG_TEXT_COMPRESSION_NONE, "Software", VERSION, sizeof(VERSION)},
		{PNG_TEXT_COMPRESSION_NONE, "Channel", desc, sizeof(desc)},
		{PNG_TEXT_COMPRESSION_NONE, "Description", "NOAA satellite image", 20}
	};

	// Parse image type
	int greyscale = 1;
	switch (chid){
		case Palleted:
			greyscale = 0;
			break;
		case Temperature:
			greyscale = 0;
			break;
		case Raw_Image: break;
		case Channel_A: break;
		case Channel_B: break;
	}

	// Parse effects
	int crop_telemetry = 0;
	for(unsigned long int i = 0; i < strlen(opts->effects); i++){
		switch (opts->effects[i]) {
			case Crop_Telemetry:
				width -= APT_TOTAL_TELE;
				offset += APT_SYNC_WIDTH + APT_SPC_WIDTH;
				crop_telemetry = 1;
				break;
			case Precipitation_Overlay:
				greyscale = 0;
				break;
			case Flip_Image: break;
			case Denoise: break;
			case Histogram_Equalise: break;
			case Linear_Equalise: break;
			case Crop_Noise: break;
			default: {
				char text[100];
				sprintf(text, "Unrecognised effect, \"%c\"", opts->effects[i]);
				warning(text);
				break;
			}
		}
	}

	FILE *pngfile;

	// Create writer
	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) {
		png_destroy_write_struct(&png_ptr, (png_infopp) NULL);
		error_noexit("Could not create a PNG writer");
		return 0;
	}
	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_write_struct(&png_ptr, (png_infopp) NULL);
		error_noexit("Could not create a PNG writer");
		return 0;
	}

	if(greyscale){
		// Greyscale image
		png_set_IHDR(png_ptr, info_ptr, width, img->nrow,
					 8, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE,
				 	 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	}else{
		// 8 bit RGB image
		png_set_IHDR(png_ptr, info_ptr, width, img->nrow,
					 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
					 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	}

	png_set_text(png_ptr, info_ptr, meta, 3);
	png_set_pHYs(png_ptr, info_ptr, 3636, 3636, PNG_RESOLUTION_METER);

	// Init I/O
	pngfile = fopen(outName, "wb");
	if (!pngfile) {
		error_noexit("Could not open PNG for writing");
		return 1;
	}
	png_init_io(png_ptr, pngfile);
	png_write_info(png_ptr, info_ptr);

	// Move prow into crow, crow ~ color rows, if required
	apt_rgb_t *crow[APT_MAX_HEIGHT];
	if(!greyscale){
		prow2crow(img->prow, img->nrow, palette, crow);
	}

	// Apply a user provided color palette
	if(chid == Palleted){
		applyUserPalette(img->prow, img->nrow, opts->palette, crow);
	}

	// Precipitation overlay
	if(CONTAINS(opts->effects, Precipitation_Overlay)){
		for(int y = 0; y < img->nrow; y++){
			for(int x = 0; x < APT_CH_WIDTH; x++){
				if(img->prow[y][x + APT_CHB_OFFSET] >= 198)
					crow[y][x + APT_CHB_OFFSET] = crow[y][x + APT_CHA_OFFSET] = apt_applyPalette(apt_PrecipPalette, img->prow[y][x + APT_CHB_OFFSET]-198);
			}
		}
	}

	printf("Writing %s", outName);

	// Float power macro (for gamma adjustment)
	#define POWF(a, b) (b == 1.0 ? a : exp(b * log(a)))
	float a = POWF(255, opts->gamma)/255;

	// Build image
	for (int y = 0; y < img->nrow; y++) {
		png_color pix[APT_IMG_WIDTH]; // Color
		png_byte mpix[APT_IMG_WIDTH]; // Mono

		int skip = 0;
		for (int x = 0; x < width; x++) {
			if(crop_telemetry && x == APT_CH_WIDTH)
				skip += APT_TELE_WIDTH + APT_SYNC_WIDTH + APT_SPC_WIDTH;

			if(greyscale){
				mpix[x] = POWF(img->prow[y][x + skip + offset], opts->gamma)/a;
			}else{
				pix[x] = (png_color){
					POWF(crow[y][x + skip + offset].r, opts->gamma)/a,
					POWF(crow[y][x + skip + offset].g, opts->gamma)/a,
					POWF(crow[y][x + skip + offset].b, opts->gamma)/a
				};
			}
		}

		if(greyscale){
			png_write_row(png_ptr, (png_bytep) mpix);
		}else{
			png_write_row(png_ptr, (png_bytep) pix);
		}
	}

	// Tidy up
	png_write_end(png_ptr, info_ptr);
	fclose(pngfile);
	printf("\nDone\n");
	png_destroy_write_struct(&png_ptr, &info_ptr);

	return 1;
}

// TODO: clean up everthing below this comment
png_structp rt_png_ptr;
png_infop rt_info_ptr;
FILE *rt_pngfile;

int initWriter(options_t *opts, apt_image_t *img, int width, int height, char *desc, char *chid){
	char outName[384];
	sprintf(outName, "%s/%s-%s.png", opts->path, img->name, chid);

	png_text meta[] = {
		{PNG_TEXT_COMPRESSION_NONE, "Software", VERSION, sizeof(VERSION)},
		{PNG_TEXT_COMPRESSION_NONE, "Channel", desc, sizeof(desc)},
		{PNG_TEXT_COMPRESSION_NONE, "Description", "NOAA satellite image", 20}
	};

	// Create writer
	rt_png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!rt_png_ptr) {
		png_destroy_write_struct(&rt_png_ptr, (png_infopp) NULL);
		error_noexit("Could not create a PNG writer");
		return 0;
	}
	rt_info_ptr = png_create_info_struct(rt_png_ptr);
	if (!rt_info_ptr) {
		png_destroy_write_struct(&rt_png_ptr, (png_infopp) NULL);
		error_noexit("Could not create a PNG writer");
		return 0;
	}

	// Greyscale image
	png_set_IHDR(rt_png_ptr, rt_info_ptr, width, height,
				 8, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE,
			 	 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	png_set_text(rt_png_ptr, rt_info_ptr, meta, 3);

	// Channel = 25cm wide
	png_set_pHYs(rt_png_ptr, rt_info_ptr, 3636, 3636, PNG_RESOLUTION_METER);

	// Init I/O
	rt_pngfile = fopen(outName, "wb");
	if (!rt_pngfile) {
		error_noexit("Could not open PNG for writing");
		return 0;
	}
	png_init_io(rt_png_ptr, rt_pngfile);
	png_write_info(rt_png_ptr, rt_info_ptr);

	// Turn off compression
	png_set_compression_level(rt_png_ptr, 0);

	return 1;
}

void pushRow(float *row, int width){
	png_byte pix[APT_IMG_WIDTH];
	for(int i = 0; i < width; i++)
		pix[i] = row[i];

	png_write_row(rt_png_ptr, (png_bytep) pix);
}

void closeWriter(){
	png_write_end(rt_png_ptr, rt_info_ptr);
	fclose(rt_pngfile);
	png_destroy_write_struct(&rt_png_ptr, &rt_info_ptr);
}
