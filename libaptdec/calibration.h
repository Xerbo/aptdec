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

#ifndef LIBAPTDEC_CALIBRATION_H_
#define LIBAPTDEC_CALIBRATION_H_

#include "algebra.h"
#include <aptdec.h>

typedef struct {
    char *name;

    // Quadratics for calculating PRT temperature
    quadratic_t prt[4];

    // Visible calibration coefficients
    struct {
        linear_t low;
        linear_t high;
        float cutoff;
    } visible[2];

    // Radiance coefficients
    struct {
        float vc, A, B;
    } rad[3];

    // Non linear correction coefficients
    struct {
        float Ns;
        quadratic_t quadratic;
    } cor[3];
} calibration_t;

// First radiation constant (mW/(m2-sr-cm-4))
static const float C1 = 1.1910427e-5f;
// Second radiation constant (cm-K)
static const float C2 = 1.4387752f;

calibration_t get_calibration(apt_satellite_t satid);

#endif
