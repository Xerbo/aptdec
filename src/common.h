/* 
 *  This file is part of Aptdec.
 *  Copyright (c) 2004-2009 Thierry Leconte (F4DWV), Xerbo (xerbo@protonmail.com) 2019-20222
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
#define VERSION "Aptdec; (c) 2004-2009 Thierry Leconte F4DWV, Xerbo (xerbo@protonmail.com) 2019-20222"

// Useful macros
#define CLIP(v, lo, hi) (v > hi ? hi : (v > lo ? v : lo))
#define CONTAINS(str, char) (strchr(str, (int) char) != NULL)

// Typedefs
#ifndef STRUCTS_DEFINED
#define STRUCTS_DEFINED
typedef struct {
	char *type; // Output image type
	char *effects; // Effects on the image
	int   satnum; // The satellite number
	char *map; // Path to a map file
	char *path; // Output directory
	int   realtime; // Realtime decoding
	char *filename; // Output filename
	char *palette; // Filename of palette
	float gamma; // Gamma
	int mapOffset;
} options_t;

enum imagetypes {
	Raw_Image='r',
	MCIR='m',
	Palleted='p',
	Temperature='t',
	Channel_A='a',
	Channel_B='b',
	Distribution='d'
};
enum effects {
	Crop_Telemetry='t',
	Histogram_Equalise='h',
	Denoise='d',
	Precipitation_Overlay='p',
	Flip_Image='f',
	Linear_Equalise='l',
	Crop_Noise='c'
};

#endif
