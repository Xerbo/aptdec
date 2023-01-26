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

#include "calibration.h"

#include "util.h"

// clang-format off
const calibration_t calibration[3] = {
    {
        .name = "NOAA-15",
        .prt = {
            { 1.36328e-06f, 0.051045f, 276.60157f }, // PRT 1
            { 1.47266e-06f, 0.050909f, 276.62531f }, // PRT 2
            { 1.47656e-06f, 0.050907f, 276.67413f }, // PRT 3
            { 1.47656e-06f, 0.050966f, 276.59258f }  // PRT 4
        },
        .visible = {
            {
                .low  = { 0.0568f,  -2.1874f },
                .high = { 0.1633f, -54.9928f },
                .cutoff = 496.0f
            }, {
                .low  = { 0.0596f,  -2.4096f },
                .high = { 0.1629f, -55.2436f },
                .cutoff = 511.0f
            }
        },
        .rad = {
            {  925.4075f, 0.337810f, 0.998719f }, // Channel 4
            {  839.8979f, 0.304558f, 0.999024f }, // Channel 5
            { 2695.9743f, 1.621256f, 0.998015f }  // Channel 3B
        },
	    .cor = {
            { -4.50f, { 0.0004524f, -0.0932f, 4.76f } }, // Channel 4
            { -3.61f, { 0.0002811f, -0.0659f, 3.83f } }, // Channel 5
            { 0.0f,   { 0.0f,        0.0f   , 0.0f  } }  // Channel 3B
	    }
    }, {
        .name = "NOAA-18",
        .prt = {
            { 1.657e-06f, 0.05090f, 276.601f }, // PRT 1
            { 1.482e-06f, 0.05101f, 276.683f }, // PRT 2
            { 1.313e-06f, 0.05117f, 276.565f }, // PRT 3
            { 1.484e-06f, 0.05103f, 276.615f }  // PRT 4
        },  
        .visible = {
            {
                .low  = { 0.06174f, -2.434f },
                .high = { 0.1841f, -63.31f },
                .cutoff = 501.54f
            }, {
                .low  = { 0.07514f, -2.960f },
                .high = { 0.2254f, -78.55f },
                .cutoff = 500.40f
            }
        },
        .rad = {
            {  928.1460f, 0.436645f, 0.998607f }, // Channel 4
            {  833.2532f, 0.253179f, 0.999057f }, // Channel 5
            { 2659.7952f, 1.698704f, 0.996960f }  // Channel 3B
        },
        .cor = {
            { -5.53f, { 0.00052337f, -0.11069f, 5.82f } }, // Channel 4
            { -2.22f, { 0.00017715f, -0.04360f, 2.67f } }, // Channel 5
            {  0.0f,  { 0.0f,         0.0f,     0.0f  } }  // Channel 3B
        }
    }, {
        .name = "NOAA-19",
        .prt = {
            { 1.405783e-06f, 0.051111f, 276.6067f }, // PRT 1
            { 1.496037e-06f, 0.051090f, 276.6119f }, // PRT 2
            { 1.496990e-06f, 0.051033f, 276.6311f }, // PRT 3
            { 1.493110e-06f, 0.051058f, 276.6268f }  // PRT 4
        },
        .visible = {
            {
                .low  = { 0.05555f, -2.159f },
                .high = { 0.1639f, -56.33f },
                .cutoff = 496.43f
            }, {
                .low  = { 0.06614f, -2.565f },
                .high = { 0.1970f, -68.01f },
                .cutoff = 500.37f
            }
        },
        .rad = {
            {  928.9f, 0.53959f, 0.998534f }, // Channel 4
            {  831.9f, 0.36064f, 0.998913f }, // Channel 5
            { 2670.0f, 1.67396f, 0.997364f }  // Channel 3B
        },
        .cor = {
            { -5.49f, { 0.00054668f, -0.11187f, 5.70f } }, // Channel 4
            { -3.39f, { 0.00024985f, -0.05991f, 3.58f } }, // Channel 5
            {  0.0f,  { 0.0f,         0.0f,     0.0f  } }  // Channel 3B
        }
    }
};

calibration_t get_calibration(aptdec_satellite_t satid) {
    switch (satid) {
        case NOAA15: return calibration[0];
        case NOAA18: return calibration[1];
        default:     return calibration[2];
    }
}
