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
#include <filter.h>

float fir( float *buff, float *coeff, int len)
{
int i;
double r;

r=0.0;
for(i=0;i<len;i++) {
	r+=buff[i]*coeff[i];
}
return r;
}


void iqfir( float *buff, float *Icoeff, float *Qcoeff, int len, float *I,float *Q)
{
int i;

*I=*Q=0.0;
for(i=0;i<len;i++) {
	register double v=buff[i];

	*I+=v*Icoeff[i];
	*Q+=v*Qcoeff[i];
}
}

float rsfir(float *buff,float *coeff,int len ,double offset ,double delta)
{
int i;
double n;
double out;

out=0.0;
for(i=0,n=offset;n<len;n+=delta,i++)
	out+=buff[i]*coeff[(int)n];

return out;
}

