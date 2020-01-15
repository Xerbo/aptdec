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

#include "offsets.h"
#include "messages.h"

typedef struct {
    float r, g, b;
} rgb_t;

extern int zenith;
extern char PrecipPalette[256*3];
extern rgb_t applyPalette(char *palette, int val);
extern rgb_t RGBcomposite(rgb_t top, float top_a, rgb_t bottom, float bottom_a);

int mapOverlay(char *filename, rgb_t **crow, int nrow, int zenith, int MCIR) {
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
	for(int y = 0; y < height; y++)
		mapRows[y] = (png_byte *) malloc(png_get_rowbytes(png, info));

	// Read image
	png_read_image(png, mapRows);

	// Tidy up
	fclose(fp);
	png_destroy_read_struct(&png, &info, NULL);

	// Map overlay / MCIR / Precipitation
	int mapOffset = (height/2)-zenith;
	for(int y = 0; y < nrow; y++) {
		for(int x = 49; x < width - 82; x++){
			// Maps are 16 bit / channel
			png_bytep px = &mapRows[CLIP(y + mapOffset, 0, height)][x * 6];
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
					float green = CLIP((map.g-256)/32.0, 0, 1);
					float blue = 1-CLIP((map.b-32)/64.0, 0, 1);
					crow[y][cha] = (rgb_t){50 + blue*50, 80 + green*70, 64};
				}else{
					// Sea
					crow[y][cha] = (rgb_t){12, 30, 85};
				}
			}

			// Color -> alpha: composite
			int composite = map.r + map.g + map.b;
			// Color -> alpha: flattern and convert to 8 bits / channel
			float factor = (255 * 255 * 2.0) / composite;
			map.r *= factor/257.0; map.g *= factor/257.0; map.b *= factor/257.0;
			// Color -> alpha: convert black to alpha
			float alpha = CLIP(composite / 65535.0, 0, 1);

			// Map overlay on channel A
			crow[y][cha] = RGBcomposite(map, alpha, crow[y][cha], 1);

			// Map overlay on channel B
			if(!MCIR)
				crow[y][chb] = RGBcomposite(map, alpha, crow[y][chb], 1);

			// Cloud overlay on channel A
			if(MCIR){
				float cloud = CLIP((crow[y][chb].r - 115) / 107, 0, 1);
				crow[y][cha] = RGBcomposite((rgb_t){240, 250, 255}, cloud, crow[y][cha], 1);
			}
		}
	}

	return 1;
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

typedef struct {
	float *prow[3000]; // Row buffers
	int nrow; // Number of rows
	int chA, chB; // ID of each channel
	char name[256]; // Stripped filename
} image_t;

typedef struct {
	char *type; // Output image type
	char *effects;
	int   satnum; // The satellite number
	char *map; // Path to a map file
	char *path; // Output directory
} options_t;

//int ImageOut(char *filename, char *chid, float **prow, int nrow, int width, int offset, char *palette, char *effects, char *mapFile) {
int ImageOut(options_t *opts, image_t *img, int offset, int width, char *desc, char *chid, char *palette){
	char outName[384];
    sprintf(outName, "%s/%s-%s.png", opts->path, img->name, chid);
	
	FILE *pngfile;

	png_text text_ptr[] = {
		{PNG_TEXT_COMPRESSION_NONE, "Software", VERSION},
		{PNG_TEXT_COMPRESSION_NONE, "Channel", desc, sizeof(desc)},
		{PNG_TEXT_COMPRESSION_NONE, "Description", "NOAA satellite image", 20}
	};

	// Reduce the width of the image to componsate for the missing telemetry
	if(opts->effects != NULL && CONTAINS(opts->effects, 't'))
		width -= TOTAL_TELE;

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
	png_set_IHDR(png_ptr, info_ptr, width, img->nrow,
	 		 	 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
	 			 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_set_text(png_ptr, info_ptr, text_ptr, 3);

	// Channel = 25cm wide
    png_set_pHYs(png_ptr, info_ptr, 3636, 3636, PNG_RESOLUTION_METER);

	// Init I/O
    pngfile = fopen(outName, "wb");
    if (!pngfile) {
		fprintf(stderr, ERR_FILE_WRITE, outName);
		return(1);
    }
    png_init_io(png_ptr, pngfile);
    png_write_info(png_ptr, info_ptr);

	// Move prow into crow, crow ~ color rows
	rgb_t *crow[img->nrow];
	for(int y = 0; y < img->nrow; y++){
		crow[y] = (rgb_t *) malloc(sizeof(rgb_t) * IMG_WIDTH);

		for(int x = 0; x < IMG_WIDTH; x++){
			if(palette == NULL){				
				crow[y][x].r = crow[y][x].g = crow[y][x].b = CLIP(img->prow[y][x], 0, 255);
			}else{
				crow[y][x] = applyPalette(palette, img->prow[y][x]);
			}
		}
	}

	// Precipitation
	// TODO: use temperature calibration for accuracy
	if(CONTAINS(opts->effects, 'p')){
		for(int y = 0; y < img->nrow; y++){
			for(int x = 0; x < CH_WIDTH; x++){
				if(img->prow[y][x + CHB_OFFSET] > 191)
					crow[y][x + CHB_OFFSET] = applyPalette(PrecipPalette, img->prow[y][x + CHB_OFFSET]);
			}
		}
	}

	if(opts->map != NULL && opts->map[0] != '\0'){
		if(mapOverlay(opts->map, crow, img->nrow, zenith, strcmp(chid, "MCIR") == 0) == 0){
			fprintf(stderr, "Skipping MCIR generation; see above.\n");
			return 0;
		}
	}else if(strcmp(chid, "MCIR") == 0){
		fprintf(stderr, "Skipping MCIR generation; no map provided.\n");
		return 0;
	}

	printf("Writing %s", outName);

	extern rgb_t falsecolor(float vis, float temp);
	int fcimage = strcmp(desc, "False Color") == 0;

	// Build RGB image
    for (int y = 0; y < img->nrow; y++) {
		png_color pix[width];

		for (int x = 0; x < width; x++) {
			if(fcimage){
				rgb_t pixel  = falsecolor(img->prow[y][x + CHA_OFFSET], img->prow[y][x + CHB_OFFSET]);
				pix[x].red   = pixel.r;
				pix[x].green = pixel.g;
				pix[x].blue  = pixel.b;
			}else{
				pix[x].red   = (int)crow[y][x + offset].r;
				pix[x].green = (int)crow[y][x + offset].g;
				pix[x].blue  = (int)crow[y][x + offset].b;
			}
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

