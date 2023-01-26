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

#ifndef LIBAPTDEC_ALGEBRA_H_
#define LIBAPTDEC_ALGEBRA_H_
#include <stddef.h>

#define M_PIf 3.14159265358979323846f
#define M_TAUf (M_PIf * 2.0f)

// A linear equation in the form of y(x) = ax + b
typedef struct {
    float a, b;
} linear_t;

// A quadratic equation in the form of y(x) = ax^2 + bx + c
typedef struct {
    float a, b, c;
} quadratic_t;

linear_t linear_regression(const float *independent, const float *dependent, size_t len);
float standard_deviation(const float *data, size_t len);
float sumf(const float *x, size_t len);
float meanf(const float *x, size_t len);
void normalizef(float *x, size_t len);

// NOTE: Modifies input array
float medianf(float *data, size_t len);

float linear_calc(float x, linear_t line);
float quadratic_calc(float x, quadratic_t quadratic);
float sincf(float x);

#endif
