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

// Constants
#define VERSION "Aptdec; (c) 2004-2009 Thierry Leconte F4DWV, Xerbo (xerbo@protonmail.com) 2019-2020"
#define MAX_HEIGHT 3000

// Useful macros
#define CLIP(v, lo, hi) (v > hi ? hi : (v > lo ? v : lo))
#define CONTAINS(str, char) (strchr(str, (int) char) != NULL)

// Typedefs
typedef struct {
	float r, g, b;
} rgb_t;
typedef struct {
	float *prow[MAX_HEIGHT]; // Row buffers
	int nrow; // Number of rows
	int chA, chB; // ID of each channel
	char name[256]; // Stripped filename
} image_t;
typedef struct {
	char *type; // Output image type
	char *effects; // Effects on the image
	int   satnum; // The satellite number
	char *map; // Path to a map file
	char *path; // Output directory
	int   realtime; // Realtime decoding
} options_t;