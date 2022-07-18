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

#include "algebra.h"
#include <math.h>

// Find the best linear equation to estimate the value of the
// dependent variable from the independent variable
linear_t linear_regression(const float *independent, const float *dependent, size_t len) {
    // Calculate mean of the dependent and independent variables (this is the centoid)
    float dependent_mean = 0.0f;
    float independent_mean = 0.0f;
    for (size_t i = 0; i < len; i++) {
        dependent_mean += dependent[i] / (float)len;
        independent_mean += independent[i] / (float)len;
    }

    // Calculate slope
    float a = 0.0f;
    {
        float a_numerator = 0.0f;
        float a_denominator = 0.0f;
        for (size_t i = 0; i < len; i++) {
            a_numerator += (independent[i] - independent_mean) * (dependent[i] - dependent_mean);
            a_denominator += powf(independent[i] - independent_mean, 2.0f);
        }
        a = a_numerator/a_denominator;
    }

    // We can now solve for the y-intercept since we know the slope
    // and the centoid, which the line must pass through
    float b = dependent_mean - a * independent_mean;

    //printf("y(x) = %fx + %f\n", a, b);
    return (linear_t){a, b};
}

float linear_calc(float x, linear_t line) {
    return x*line.a + line.b;
}

float quadratic_calc(float x, quadratic_t quadratic) {
    return x*x*quadratic.a + x*quadratic.b + quadratic.c;
}
