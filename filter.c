/*
 *  Atpdec
 *  Copyright (c) 2003 by Thierry Leconte (F4DWV)
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
  float
fir (float *buff, const float *coeff, const int len) 
{
  int i;
  double r;
  r = 0.0;
  for (i = 0; i < len; i++) {
    r += buff[i] * coeff[i];
  }
  return r;
}
void
iqfir (float *buff, const float *Icoeff, const float *Qcoeff, const int len,
       float *Iptr, float *Qptr) 
{
  int i;
  float I, Q;
  I = Q = 0.0;
  for (i = 0; i < len; i++) {
    double v;
    v = buff[i];
    I += v * Icoeff[i];
    Q += v * Qcoeff[i];
  } *Iptr = I;
  *Qptr = Q;
} float
rsfir (float *buff, const float *coeff, const int len, const double offset,
       const double delta) 
{
  int i;
  double n;
  double out;
  out = 0.0;
  for (i = 0, n = offset; n < len - 1; n += delta, i++) {
    int k;
    double alpha;
    k = (int) n;
    alpha = n - k;
    out += buff[i] * (coeff[k] * (1.0 - alpha) + coeff[k + 1] * alpha);
  } return out;
}
double
iir (double x, iirbuff_t * buff, const iircoeff_t * coeff) 
{
  buff->x[4] = buff->x[3];
  buff->x[3] = buff->x[2];
  buff->x[2] = buff->x[1];
  buff->x[1] = buff->x[0];
  buff->x[0] = x / coeff->G;
  buff->y[2] = buff->y[1];
  buff->y[1] = buff->y[0];
  buff->y[0] = buff->x[4] 
    +coeff->x[2] * buff->x[3] 
    +coeff->x[1] * buff->x[2] 
    +coeff->x[0] * buff->x[1] 
    +buff->x[0]  +coeff->y[1] * buff->y[2]  +coeff->y[0] * buff->y[1];
  return (buff->y[0]);
}


