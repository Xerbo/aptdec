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

#include "util.h"

#include <stdlib.h>
#include <math.h>

float clamp(float x, float lo, float hi) {
    if (x > hi) return hi;
    if (x < lo) return lo;
    return x;
}

int clamp_int(int x, int lo, int hi) {
    if (x > hi) return hi;
    if (x < lo) return lo;
    return x;
}

void swap_uint8(uint8_t *a, uint8_t *b) {
    uint8_t tmp = *a;
    *a = *b;
    *b = tmp;
}
