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


typedef struct {
double slope;
double offset;
} rgparam;

static void rgcomp(double *x, rgparam *rgpr)
{
/*const double y[9] = { 0.106,0.215,0.324,0.434,0.542,0.652,0.78,0.87 ,0.0}; */
const double y[9] = { 0.11872,0.2408,0.36288,0.48608,0.60704,0.73024,0.8736,0.9744 , 0.0 }; 
const double yavg=0.48819556;
double xavg;
double sxx,sxy;
int i;

for(i=0,xavg=0.0;i<9;i++)
	xavg+=x[i];
xavg/=9;
for(i=0,sxx=0.0;i<9;i++) {
	float t=x[i]-xavg;
	sxx+=t*t;
}
for(i=0,sxy=0.0;i<9;i++) {
	sxy+=(x[i]-xavg)*(y[i]-yavg);
}
rgpr->slope=sxy/sxx;
rgpr->offset=yavg-rgpr->slope*xavg;

}

static double rgcal(float x,rgparam *rgpr)
{
return(rgpr->slope*x+rgpr->offset);
}

int Calibrate(float **prow,int nrow,int offset)
{

double tele[3000];
double frm[9];
rgparam regr[30];
int n;
int k,mk;
int shift;
int mshift,channel;
float max;


printf("Calibration ");
fflush(stdout);

/* build telemetry values lines */
for(n=0;n<nrow;n++) {
	int i;

	tele[n]=0.0;
	for(i=3;i<43;i++)
		tele[n]+=prow[n][i+offset+909];
	tele[n]/=40;
}

if(nrow<128) {
	fprintf(stderr," impossible, not enought row\n");
	return (0);
}

/* find telemetry start */
max=-1000.0;mshift=0;
for(n=60;n<nrow-3;n++) {
	float df;

	df=(tele[n-4]+tele[n-3]+tele[n-2]+tele[n-1])-(tele[n]+tele[n+1]+tele[n+2]+tele[n+3]);
	if(df> max){ mshift=n; max=df; }
}

mshift-=64;
shift=mshift%128;
if(nrow<shift+8*9) {
	fprintf(stderr," impossible, not enought row\n");
	return (0);
}

/* compute regression params */
for(n=shift,k=0;n<nrow-8*9;n+=128,k++) {
	int j;

	for(j=0;j<9;j++) {
		int i;

		frm[j]=0.0;
		for(i=1;i<7;i++)
			frm[j]+=tele[n+j*8+i];
		frm[j]/=6;
	}
	rgcomp(frm,&(regr[k]));


	if(n==mshift) {
		/* channel ID */
		int i;
		float cv,df;

		for(i=1,cv=0;i<7;i++)
			cv+=tele[n+15*8+i];
		cv/=6;
		
		for(i=0,max=10000.0,channel=-1;i<6;i++) {
			df=cv-frm[i]; df=df*df;
			if (df<max) { channel=i; max=df; }
		}
	}
}
mk=k;

for(n=0;n<nrow;n++) {
	float *pixelv;
	int	i,c;

	pixelv=prow[n];
	for(i=0;i<954;i++) {
		float pv;
		int k,kof;

		pv=pixelv[i+offset];
		k=(n-shift)/128;
		kof=(n-shift)%128;
		if(kof<64) {
			if(k<1)
		 		pv=rgcal(pv,&(regr[k]));
			else
		 		pv=rgcal(pv,&(regr[k]))*(64+kof)/128.0+
		 		   rgcal(pv,&(regr[k-1]))*(64-kof)/128.0;
		} else {
			if((k+1)>=mk)
		 		pv=rgcal(pv,&(regr[k]));
			else
		 		pv=rgcal(pv,&(regr[k]))*(192-kof)/128.0+
		 		   rgcal(pv,&(regr[k+1]))*(kof-64)/128.0;
		}

		if(pv>1.0) pv=1.0;
		if(pv<0.0) pv=0.0;
		pixelv[i+offset]=pv;
	}
}
printf("Done\n");
return (channel);
}
