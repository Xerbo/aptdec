/*
 *  Aptdec
 *  Copyright (c) 2004 by Thierry Leconte (F4DWV)
 *
 *      $Id$
 *
 *   This library is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details.
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#include "filter.h"
#include <math.h>

// Sum of a matrix multiplication of 2 arrays
float fir(float *buff, const float *coeff, const int len) {
    double r;

    r = 0.0;
    for (int i = 0; i < len; i++) {
	    r += buff[i] * coeff[i];
    }
    return(r);
}

// Create an IQ sample from a sample buffer
void iqfir(float *buff, const float *coeff, const int len, double *I, double *Q) {
    double i, q;

    i = q = 0.0;
    for (int k = 0; k < len; k++) {
        q += buff[2*k] * coeff[k];
        // Average out the I samples, which gives us the DC offset
        i += buff[2*k];
    }
    // Grab the peak value of the wave and subtract the DC offset
    i = buff[len-1] - (i / len);
    *I = i, *Q = q;
}

// Denoise, I don't know how it works, but it does
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
