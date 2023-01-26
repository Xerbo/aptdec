/*
 * aptdec - A lightweight FOSS (NOAA) APT decoder
 * Copyright (C) 2019-2023 Xerbo (xerbo@protonmail.com)
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

#ifndef APTDEC_CLI_PNGIO_H_
#define APTDEC_CLI_PNGIO_H_

#include <aptdec.h>
#include <png.h>

typedef struct {
    png_structp png;
    png_infop info;
    FILE *file;
    aptdec_region_t region;
} writer_t;

writer_t *writer_init(const char *filename, aptdec_region_t region, uint32_t height, int color, char *channel);
void writer_free(writer_t *png);

void writer_write_image(writer_t *png, const aptdec_image_t *img);
void writer_write_image_gradient(writer_t *png, const aptdec_image_t *img, const uint32_t *gradient);
void writer_write_image_lut(writer_t *png, const aptdec_image_t *img, const png_colorp lut);

int read_lut(const char *filename, png_colorp out);

#endif
