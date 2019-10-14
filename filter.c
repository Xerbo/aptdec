/*
 *  Aptec
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

float fir(float *buff, const float *coeff, const int len) {
    int i;
    double r;

    r = 0.0;
    for (i = 0; i < len; i++) {
	    r += buff[i] * coeff[i];
    }
    return r;
}

void iqfir(float *buff, const float *coeff, const int len, double *I, double *Q) {
    int k;
    double i, q;

    i = q = 0.0;
    for (k = 0; k < len; k++) {
        q += buff[2*k] * coeff[k];
        i += buff[2*k];
    }
    i= buff[len-1] - i / len;
    *I=i, *Q=q;
}

float rsfir(double *buff, const float *coeff, const int len, const double offset, const double delta) {
    int i;
    double n;
    double out;

    out = 0.0;
    for (i = 0, n = offset; i < (len-1)/delta-1; n += delta, i++) {
        int k;
        double alpha;

        k = (int)floor(n);
        alpha = n - k;
        out += buff[i] * (coeff[k] * (1.0 - alpha) + coeff[k + 1] * alpha);
    }
    return out;
}

