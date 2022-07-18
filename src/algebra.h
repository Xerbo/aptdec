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

#ifndef APTDEC_ALGEBRA_H
#define APTDEC_ALGEBRA_H
#include <stddef.h>

// A linear equation in the form of y(x) = ax + b
typedef struct {
    float a, b;
} linear_t;

// A quadratic equation in the form of y(x) = ax^2 + bx + c
typedef struct {
    float a, b, c;
} quadratic_t;

linear_t linear_regression(const float *independent, const float *dependent, size_t len);

float linear_calc(float x, linear_t line);
float quadratic_calc(float x, quadratic_t line);

#endif
