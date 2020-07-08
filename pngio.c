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

#include <png.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#include "common.h"
#include "offsets.h"

extern char PrecipPalette[256*3];
extern rgb_t applyPalette(char *palette, int val);
extern rgb_t RGBcomposite(rgb_t top, float top_a, rgb_t bottom, float bottom_a);

int mapOverlay(char *filename, rgb_t **crow, int nrow, int zenith, int MCIR) {
	FILE *fp = fopen(filename, "rb");
	if(!fp) {
		fprintf(stderr, "Cannot open %s\n", filename);
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
	for(int y = 0; y < height; y++)
		mapRows[y] = (png_byte *) malloc(png_get_rowbytes(png, info));

	// Read image
	png_read_image(png, mapRows);

	// Tidy up
	fclose(fp);
	png_destroy_read_struct(&png, &info, NULL);

	printf("Adding map overlay\n");

	// Map overlay / MCIR / Precipitation
	int mapOffset = (height/2)-zenith;
	for(int y = 0; y < nrow; y++) {
		for(int x = 49; x < width - 82; x++){
			// Maps are 16 bit / channel
			png_bytep px = &mapRows[CLIP(y + mapOffset, 0, height-1)][x * 6];
			rgb_t map = {
				(px[0] << 8) | px[1],
				(px[2] << 8) | px[3],
				(px[4] << 8) | px[5]
			};

			// Pixel offsets
			int chb = x + CHB_OFFSET - 49;
			int cha = x + 36;

			// Fill in map
			if(MCIR){
				if(map.b < 128 && map.g > 128){
					// Land
					float darken = ((255-crow[y][chb].r)-100)/50;
					float green = CLIP(map.g/300, 0, 1);
					float blue = 0.15 - CLIP(map.b/960.0, 0, 1);
					crow[y][cha] = (rgb_t){blue*1000*darken, green*98*darken, blue*500.0*darken};
				}else{
					// Sea
					crow[y][cha] = (rgb_t){9, 17, 74};
				}
			}

			// Color -> alpha: composite
			int composite = map.r + map.g + map.b;
			// Color -> alpha: flattern and convert to 8 bits / channel
			float factor = (255 * 255 * 2.0) / composite;
			map.r *= factor/257.0; map.g *= factor/257.0; map.b *= factor/257.0;
			// Color -> alpha: convert black to alpha
			float alpha = CLIP(composite / 65535.0, 0, 1);
			// Clip
			map.r = CLIP(map.r, 0, 255.0);
			map.g = CLIP(map.g, 0, 255.0);
			map.b = CLIP(map.b, 0, 255.0);

			// Map overlay on channel A
			crow[y][cha] = RGBcomposite(map, alpha, crow[y][cha], 1);
			// Map overlay on channel B
			if(!MCIR)
				crow[y][chb] = RGBcomposite(map, alpha, crow[y][chb], 1);

			// Cloud overlay on channel A
			if(MCIR){
				float cloud = CLIP((crow[y][chb].r - 113) / 90.0, 0, 1);
				crow[y][cha] = RGBcomposite((rgb_t){255, 250, 245}, cloud, crow[y][cha], 1);
			}
		}
	}

	return 1;
}

int readRawImage(char *filename, float **prow, int *nrow) {
	FILE *fp = fopen(filename, "r");
	if(!fp) {
		fprintf(stderr, "Cannot open %s\n", filename);
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
	if(width != IMG_WIDTH){
		fprintf(stderr, "Raw image must be %ipx wide.\n", IMG_WIDTH);
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

int readPalette(char *filename, rgb_t **pixels) {
	FILE *fp = fopen(filename, "r");
	if(!fp) {
		fprintf(stderr, "Cannot open %s\n", filename);
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
		fprintf(stderr, "Palette must be 256x256.\n");
		return 0;
	}else if(bit_depth != 8){
		fprintf(stderr, "Palette must be 8 bit color.\n");
		return 0;
	}else if(color_type != PNG_COLOR_TYPE_RGB){
		fprintf(stderr, "Palette must be RGB.\n");
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
		pixels[y] = (rgb_t *) malloc(sizeof(rgb_t) * width);

		for(int x = 0; x < width; x++)
			pixels[y][x] = (rgb_t){
				PNGrows[y][x*3],
				PNGrows[y][x*3 + 1], 
				PNGrows[y][x*3 + 2]
			};
	}

	return 1;
}

void prow2crow(float **prow, int nrow, char *palette, rgb_t **crow){
	for(int y = 0; y < nrow; y++){
		crow[y] = (rgb_t *) malloc(sizeof(rgb_t) * IMG_WIDTH);

		for(int x = 0; x < IMG_WIDTH; x++){
			if(palette == NULL)
				crow[y][x].r = crow[y][x].g = crow[y][x].b = prow[y][x];
			else
				crow[y][x] = applyPalette(palette, prow[y][x]);
		}
	}
}

int applyUserPalette(float **prow, int nrow, char *filename, rgb_t **crow){
	rgb_t *pal_row[256];
	if(!readPalette(filename, pal_row)){
		fprintf(stderr, "Could not read palette\n");
		return 0;
	}

	for(int y = 0; y < nrow; y++){
		for(int x = 0; x < CH_WIDTH; x++){
			int cha = CLIP(prow[y][x + CHA_OFFSET], 0, 255);
			int chb = CLIP(prow[y][x + CHB_OFFSET], 0, 255);
			crow[y][x + CHA_OFFSET] = pal_row[chb][cha];
		}
	}
	
	return 1;
}

int ImageOut(options_t *opts, image_t *img, int offset, int width, char *desc, char chid, char *palette){
	char outName[512];
	if(opts->filename == NULL || opts->filename[0] == '\0'){
		sprintf(outName, "%s/%s-%c.png", opts->path, img->name, chid);
	}else{
		sprintf(outName, "%s/%s", opts->path, opts->filename);
	}

	png_text meta[] = {
		{PNG_TEXT_COMPRESSION_NONE, "Software", VERSION},
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
		case MCIR:
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
				width -= TOTAL_TELE;
				offset += SYNC_WIDTH + SPC_WIDTH;
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
			default:
				fprintf(stderr, "NOTICE: Unrecognised effect, \"%c\"\n", opts->effects[i]);
				break;
		}
	}

	if(opts->map != NULL && opts->map[0] != '\0'){
		greyscale = 0;
	}

	FILE *pngfile;

	// Create writer
	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) {
		png_destroy_write_struct(&png_ptr, (png_infopp) NULL);
		fprintf(stderr, "Could not create a PNG writer\n");
		return 0;
	}
	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_write_struct(&png_ptr, (png_infopp) NULL);
		fprintf(stderr, "Could not create a PNG writer\n");
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
		fprintf(stderr, "Could not open %s for writing\n", outName);
		return 1;
	}
	png_init_io(png_ptr, pngfile);
	png_write_info(png_ptr, info_ptr);

	// Move prow into crow, crow ~ color rows, if required
	rgb_t *crow[MAX_HEIGHT];
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
			for(int x = 0; x < CH_WIDTH; x++){
				if(img->prow[y][x + CHB_OFFSET] >= 198)
					crow[y][x + CHB_OFFSET] = crow[y][x + CHA_OFFSET] = applyPalette(PrecipPalette, img->prow[y][x + CHB_OFFSET]-198);
			}
		}
	}

	// Map stuff
	if(opts->map != NULL && opts->map[0] != '\0'){
		if(!mapOverlay(opts->map, crow, img->nrow, img->zenith, CONTAINS(opts->type, MCIR))){
			fprintf(stderr, "Skipping MCIR generation.\n");
			return 0;
		}
	}else if(CONTAINS(opts->type, MCIR)){
		fprintf(stderr, "Skipping MCIR generation; no map provided.\n");
		return 0;
	}

	printf("Writing %s", outName);

	// Float power macro (for gamma adjustment)
	#define POWF(a, b) (b == 1.0 ? a : exp(b * log(a)))
	float a = POWF(255, opts->gamma)/255;

	// Build image
	for (int y = 0; y < img->nrow; y++) {
		png_color pix[width]; // Color
		png_byte mpix[width]; // Mono

		int skip = 0;
		for (int x = 0; x < width; x++) {
			if(crop_telemetry && x == CH_WIDTH)
				skip += TELE_WIDTH + SYNC_WIDTH + SPC_WIDTH;

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

int initWriter(options_t *opts, image_t *img, int width, int height, char *desc, char *chid){
	char outName[384];
	sprintf(outName, "%s/%s-%s.png", opts->path, img->name, chid);

	png_text meta[] = {
		{PNG_TEXT_COMPRESSION_NONE, "Software", VERSION},
		{PNG_TEXT_COMPRESSION_NONE, "Channel", desc, sizeof(desc)},
		{PNG_TEXT_COMPRESSION_NONE, "Description", "NOAA satellite image", 20}
	};

	// Create writer
	rt_png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!rt_png_ptr) {
		png_destroy_write_struct(&rt_png_ptr, (png_infopp) NULL);
		fprintf(stderr, "Could not create a PNG writer\n");
		return 0;
	}
	rt_info_ptr = png_create_info_struct(rt_png_ptr);
	if (!rt_info_ptr) {
		png_destroy_write_struct(&rt_png_ptr, (png_infopp) NULL);
		fprintf(stderr, "Could not create a PNG writer\n");
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
		fprintf(stderr, "Could not open %s for writing\n", outName);
		return 0;
	}
	png_init_io(rt_png_ptr, rt_pngfile);
	png_write_info(rt_png_ptr, rt_info_ptr);

	// Turn off compression
	png_set_compression_level(rt_png_ptr, 0);

	return 1;
}

void pushRow(float *row, int width){
	png_byte pix[width];
	for(int i = 0; i < width; i++)
		pix[i] = row[i];

	png_write_row(rt_png_ptr, (png_bytep) pix);
}

void closeWriter(){
	png_write_end(rt_png_ptr, rt_info_ptr);
	fclose(rt_pngfile);
	png_destroy_write_struct(&rt_png_ptr, &rt_info_ptr);
}
