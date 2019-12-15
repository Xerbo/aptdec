/* 
 *  This file is part of Aptdec.
 *  Copyright (c) 2004-2009 Thierry Leconte (F4DWV), Xerbo (xerbo@protonmail.com) 2019
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

#include <math.h>

#include "filter.h"

// Finite impulse response
float fir(float *buff, const float *coeff, const int len) {
    double r;

    r = 0.0;
    for (int i = 0; i < len; i++) {
	    r += buff[i] * coeff[i];
    }
    return(r);
}

/* IQ finite impulse response
 * Turn samples into a single IQ sample
 */
void iqfir(float *buff, const float *coeff, const int len, double *I, double *Q) {
    double i, q;

    i = q = 0.0;
    for (int k = 0; k < len; k++) {
        q += buff[2*k] * coeff[k];
        i += buff[2*k];
    }
    i = buff[len-1] - (i / len);
    *I = i, *Q = q;
}

/* Gaussian finite impulse responce compensation
 * https://www.recordingblogs.com/wiki/gaussian-window
 */
float rsfir(double *buff, const float *coeff, const int len, const double offset, const double delta) {
    double out;

    out = 0.0;
    double n = offset;
    for (int i = 0; i < (len-1)/delta-1; n += delta, i++) {
        int k;
        double alpha;

        k = (int)floor(n);
        alpha = n - k;
        out += buff[i] * (coeff[k] * (1.0 - alpha) + coeff[k + 1] * alpha);
    }
    return(out);
}
