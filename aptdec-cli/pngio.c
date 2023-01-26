/*
 * aptdec - A lightweight FOSS (NOAA) APT decoder
 * Copyright (C) 2004-2009 Thierry Leconte (F4DWV) 2019-2023 Xerbo (xerbo@protonmail.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "pngio.h"

#include <png.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

writer_t *writer_init(const char *filename, aptdec_region_t region, uint32_t height, int color, char *channel) {
    writer_t *png = calloc(1, sizeof(writer_t));
    png->region = region;

    // Create writer
    png->png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png->png) {
        error_noexit("Could not create PNG write struct");
        return NULL;
    }

    png->info = png_create_info_struct(png->png);
    if (!png->info) {
        error_noexit("Could not create PNG info struct");
        return NULL;
    }

    png_set_IHDR(png->png, png->info, png->region.width, height, 8, color, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    char version[128] = { 0 };
    int version_len = get_version(version);

    png_text text[] = {
        {PNG_TEXT_COMPRESSION_NONE, "Software", version, version_len},
        {PNG_TEXT_COMPRESSION_NONE, "Channel", channel, strlen(channel)}
    };
    png_set_text(png->png, png->info, text, 2);

    png->file = fopen(filename, "wb");
    if (!png->file) {
        error_noexit("Could not open PNG file");
        return NULL;
    }
    png_init_io(png->png, png->file);
    png_write_info(png->png, png->info);

    printf("Writing %s...\n", filename);
    return png;
}

void writer_write_image(writer_t *png, const aptdec_image_t *img) {
    for (size_t y = 0; y < img->rows; y++) {
        png_write_row(png->png, &img->data[y*APTDEC_IMG_WIDTH + png->region.offset]);
    }
}

void writer_write_image_gradient(writer_t *png, const aptdec_image_t *img, const uint32_t *gradient) {
    for (size_t y = 0; y < img->rows; y++) {
        png_color pixels[APTDEC_IMG_WIDTH];
        for (size_t x = 0; x < APTDEC_IMG_WIDTH; x++) {
            aptdec_rgb_t pixel = aptdec_gradient(gradient, img->data[y*APTDEC_IMG_WIDTH + x + png->region.offset]);
            pixels[x] = (png_color){ pixel.r, pixel.g, pixel.b };
        }

        png_write_row(png->png, (png_bytep)pixels);
    }
}

void writer_write_image_lut(writer_t *png, const aptdec_image_t *img, const png_colorp lut) {
    for (size_t y = 0; y < img->rows; y++) {
        png_color pixels[APTDEC_CH_WIDTH];
        for (size_t x = 0; x < APTDEC_CH_WIDTH; x++) {
            uint8_t a = img->data[y*APTDEC_IMG_WIDTH + x + APTDEC_CHA_OFFSET];
            uint8_t b = img->data[y*APTDEC_IMG_WIDTH + x + APTDEC_CHB_OFFSET];
            pixels[x] = lut[b*256 + a];
        }

        png_write_row(png->png, (png_bytep)pixels);
    }
}

void writer_free(writer_t *png) {
    png_write_end(png->png, png->info);
    png_destroy_write_struct(&png->png, &png->info);
    fclose(png->file);
    free(png);
}

int read_lut(const char *filename, png_colorp out) {
    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) {
        error_noexit("Could not create PNG read struct");
        return 0;
    }
    png_infop info = png_create_info_struct(png);
    if (!info) {
        error_noexit("Could not create PNG info struct");
        return 0;
    }

    FILE *file = fopen(filename, "rb");
    if (!file) {
        error_noexit("Cannot open LUT");
        return 0;
    }

    png_init_io(png, file);

    // Read metadata
    png_read_info(png, info);
    uint32_t width = png_get_image_width(png, info);
    uint32_t height = png_get_image_height(png, info);
    png_byte color_type = png_get_color_type(png, info);
    png_byte bit_depth = png_get_bit_depth(png, info);

    if (width != 256 && height != 256) {
        error_noexit("LUT must be 256x256");
        return 0;
    }
    if (bit_depth != 8) {
        error_noexit("LUT must be 8 bit");
        return 0;
    }
    if (color_type != PNG_COLOR_TYPE_RGB) {
        error_noexit("LUT must be RGB");
        return 0;
    }

    for (uint32_t i = 0; i < height; i++) {
        png_read_row(png, (png_bytep)&out[i*width], NULL);
    }

    png_read_end(png, info);
    png_destroy_read_struct(&png, &info, NULL);
    fclose(file);
    return 1;
}
