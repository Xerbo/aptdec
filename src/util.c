/*
 * aptdec - A lightweight FOSS (NOAA) APT decoder
 * Copyright (C) 2019-2022 Xerbo (xerbo@protonmail.com)
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

#include "util.h"

#include <stdio.h>
#include <stdlib.h>

void error_noexit(const char *text) {
#ifdef _WIN32
    fprintf(stderr, "Error: %s\r\n", text);
#else
    fprintf(stderr, "\033[31mError: %s\033[0m\n", text);
#endif
}
void error(const char *text) {
    error_noexit(text);
    exit(1);
}
void warning(const char *text) {
#ifdef _WIN32
    fprintf(stderr, "Warning: %s\r\n", text);
#else
    fprintf(stderr, "\033[33mWarning: %s\033[0m\n", text);
#endif
}

float clamp(float x, float hi, float lo) {
    if (x > hi) return hi;
    if (x < lo) return lo;
    return x;
}

float clamp_half(float x, float hi) { return clamp(x, hi, -hi); }
