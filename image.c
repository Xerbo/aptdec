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

#include <stdio.h>
#include <string.h>
#include <sndfile.h>

int Calibrate(float **prow,int nrow,int offset)
{
/* const double cal[8] = { 0.106,0.215,0.324,0.434,0.542,0.652,0.78,0.87 }; */
const double cal[8] = { 0.1155 ,0.234,0.353,0.473,0.59,0.71,0.85,0.95 };

float tele[2000];
float frm[16];
float slv[8];
int n,i;
int shift,channel;
float max;


printf("Calibration ");
fflush(stdout);

if(nrow<128) {
	fprintf(stderr," impossible, not enought row\n");
	return (0);
}

/* build telemetry values */
for(n=0;n<nrow;n++) {
	tele[n]=0.0;
	for(i=3;i<43;i++)
		tele[n]+=prow[n][i+offset+909];
	tele[n]/=40;
}

max=0.0;shift=0;
for(n=60;n<nrow-64;n++) {
	float df;

	df=(tele[n-4]+tele[n-3]+tele[n-2]+tele[n-1])-(tele[n]+tele[n+1]+tele[n+2]+tele[n+3]);
	if(df> max){ shift=n; max=df; }
}
for(n=0;n<16;n++) {
	frm[n]=0.0;
	for(i=1;i<7;i++)
		frm[n]+=tele[shift-64+n*8+i];
	frm[n]/=6;
}

/* channel I.D */
max=100000.0;
for(n=0;n<6;n++) {
	float df;
	df=fabs(frm[n]-frm[15]);
	if(df<max) { 
		max=df;
		channel=n;
	}
}
fflush(stdout);

/* compute slopes */
slv[0]=cal[0]/(frm[0]-frm[8]);
for(n=1;n<8;n++) {
	slv[n]=(cal[n]-cal[n-1])/(frm[n]-frm[n-1]);
}

for(n=0;n<nrow;n++) {
	float *pixelv;
	int	i,c;

	pixelv=prow[n];
	for(i=0;i<909;i++) {
		float pv;

		pv=pixelv[i+offset];

		if(pv<frm[8]) {pv=0.0; goto conv; }
		if(pv<frm[0]) {pv=(pv-frm[8])*slv[0]; goto conv; }
		for(c=1;c<8;c++) 
		    if(pv<frm[c]) {pv=(pv-frm[c-1])*slv[c]+cal[c-1]; goto conv; }
 	        pv=(pv-frm[7])*slv[7]+cal[7];
		if(pv>1.0) pv=1.0;
conv:
		pixelv[i+offset]=pv;
	}
}
printf("Done\n");
return (channel);
}
