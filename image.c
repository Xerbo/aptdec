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
#include <math.h>

typedef struct {
double slope;
double offset;
} rgparam;

static void rgcomp(double x[16], rgparam *rgpr)
{
/* 0.106,0.215,0.324,0.434,0.542,0.652,0.78,0.87 ,0.0 */
const double y[9] = { 31.1,63.0,95.0,127.2,158.9,191.1,228.6,255.0, 0.0 }; 
const double yavg=(y[0]+y[1]+y[2]+y[4]+y[5]+y[6]+y[7]+y[8])/9.0;
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


static double tele[16];
static nbtele;

int Calibrate(float **prow,int nrow,int offset)
{

double teleline[3000];
double wedge[16];
rgparam regr[30];
int n;
int k;
int mtelestart,telestart;
int channel=-1;
float max;

printf("Calibration ");
fflush(stdout);

/* build telemetry values lines */
for(n=0;n<nrow;n++) {
	int i;

	teleline[n]=0.0;
	for(i=3;i<43;i++)
		teleline[n]+=prow[n][i+offset+909];
	teleline[n]/=40;
}

if(nrow<192) {
	fprintf(stderr," impossible, not enought row\n");
	return (0);
}

/* find telemetry start */
max=0.0;mtelestart=0;
for(n=nrow/3-64;n<2*nrow/3-64;n++) {
	float df;

	df=(teleline[n-4]+teleline[n-3]+teleline[n-2]+teleline[n-1])/(teleline[n]+teleline[n+1]+teleline[n+2]+teleline[n+3]);
	if(df> max){ mtelestart=n; max=df; }
}

mtelestart-=64;
telestart=mtelestart%128;

if(mtelestart <0 || nrow<telestart+128) {
	fprintf(stderr," impossible, not enought row\n");
	return (0);
}

/* compute wedges and regression */
for(n=telestart,k=0;n<nrow-128;n+=128,k++) {
	int j;

	for(j=0;j<16;j++) {
		int i;

		wedge[j]=0.0;
		for(i=1;i<7;i++)
			wedge[j]+=teleline[n+j*8+i];
		wedge[j]/=6;
	}

	rgcomp(wedge,&(regr[k]));

	if (k==1) {
		/* channel ID */
		for(j=0,max=10000.0,channel=-1;j<6;j++) {
			float df;
			df=wedge[15]-wedge[j]; df=df*df;
			if (df<max) { channel=j; max=df; }
		}
		for(j=0;j<16;j++)
			tele[j]=rgcal(wedge[j],&(regr[k]));
	}
}
nbtele=k;

/* calibrate */
for(n=0;n<nrow;n++) {
	float *pixelv;
	int	i,c;

	pixelv=prow[n];
	for(i=0;i<954;i++) {
		float pv;
		int k,kof;

		pv=pixelv[i+offset];

		k=(n-telestart)/128;
		if(k>=nbtele) k=nbtele-1;
		kof=(n-telestart)%128;
		if(kof<64) {
			if(k<1) {
				pv=rgcal(pv,&(regr[k]));
			} else {
	 			pv=rgcal(pv,&(regr[k]))*(64+kof)/128.0+
	 		   	rgcal(pv,&(regr[k-1]))*(64-kof)/128.0;
			}
		} else {
			if((k+1)>=nbtele) {
				pv=rgcal(pv,&(regr[k]));
			} else {
	 			pv=rgcal(pv,&(regr[k]))*(192-kof)/128.0+
	 		   	rgcal(pv,&(regr[k+1]))*(kof-64)/128.0;
			}
		}

		if(pv>255.0) pv=255.0;
		if(pv<0.0) pv=0.0;
		pixelv[i+offset]=pv;
	}
}
printf("Done\n");
return(channel);
}

/* ------------------------------temperature calibration -----------------------*/
extern int satnum;
const struct {
        float d[4][3];
        struct {
                float vc,A,B;
        } rad[3];
        struct {
                float Ns;
                float b[3];
        } cor[3];
} satcal[2]=
#include "satcal.h"

typedef struct {
double  Nbb;
double  Cs;
double  Cb;
int ch;
} tempparam;

/* temperature compensation for IR channel */
static void tempcomp(double t[16], int ch,tempparam *tpr)
{
double Tbb,T[4];
double C;
double r;
int n;

	tpr->ch=ch-3;

	/* compute equivalent T black body  */
	for (n=0;n<4;n++) {
		float d0,d1,d2;

		C=t[9+n]*4;
		d0=satcal[satnum].d[n][0];
		d1=satcal[satnum].d[n][1];
		d2=satcal[satnum].d[n][2];
		T[n]=d0;
		T[n]+=d1*C; C=C*C; 
		T[n]+=d2*C;
	}
	Tbb=(T[0]+T[1]+T[2]+T[3])/4.0;
	Tbb=satcal[satnum].rad[tpr->ch].A+satcal[satnum].rad[tpr->ch].B*Tbb;

	/* compute radiance Black body */
	C=satcal[satnum].rad[tpr->ch].vc;
	tpr->Nbb=c1*C*C*C/(exp(c2*C/Tbb)-1.0);
	/* store Count Blackbody and space */
	tpr->Cs=1023; /* don't know how to get it in the APT telemetry any idea ? */
	tpr->Cb=t[14]*4.0;
}

static double tempcal(float Ce,tempparam *rgpr)
{
double Nl,Nc,Ns,Ne;
double T,vc;

	Ns=satcal[satnum].cor[rgpr->ch].Ns;
	Nl=Ns+(rgpr->Nbb-Ns)*(rgpr->Cs-Ce*4.0)/(rgpr->Cs-rgpr->Cb);
	Nc=satcal[satnum].cor[rgpr->ch].b[0]+
	   satcal[satnum].cor[rgpr->ch].b[1]*Nl+
	   satcal[satnum].cor[rgpr->ch].b[2]*Nl*Nl;

	Ne=Nl+Nc;

	vc=satcal[satnum].rad[rgpr->ch].vc;
	T=c2*vc/log(c1*vc*vc*vc/Ne+1.0);
	T=(T-satcal[satnum].rad[rgpr->ch].A)/satcal[satnum].rad[rgpr->ch].B;

	/* rescale to range 0-255 for -60-+40 °C */
	T=(T-273.15+60)/100*255.0;

	return(T);
}
int Temperature(float **prow,int nrow,int channel,int offset)
{
tempparam temp;
int n;

printf("Temperature ");

	tempcomp(tele,channel,&temp);


for(n=0;n<nrow;n++) {
	float *pixelv;
	int	i,c;

	pixelv=prow[n];
	for(i=0;i<954;i++) {
		float pv;

		pv=tempcal(pixelv[i+offset],&temp);

		if(pv>255.0) pv=255.0;
		if(pv<0.0) pv=0.0;
		pixelv[i+offset]=pv;
	}
}
printf("Done\n");
}
