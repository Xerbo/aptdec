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
#include <math.h>
#include <filter.h>
#include <filtercoeff.h>

#define Fe 11025
#define PixelLine 2080
#define Fp (2*PixelLine)
#define RSMULT 265
#define Fi (Fp*RSMULT)

static double FreqOsc=2.0*2400.0/Fe;
static double FreqLine=1.0;
extern int getsample(float *inbuff ,int nb);

static float pll(float Iv, float Qv)
{
/* pll coeff */
#define K1 0.01
#define K2 5e-6

static double PhaseOsc=0.0;

double DPhi;
double DF;
double Phi;
double I,Q;
double Io,Qo;

/* quadrature oscillator */
Phi=M_PI*PhaseOsc;
Io=cos(Phi); Qo=sin(Phi);

/* phase detector */
I=Iv*Io+Qv*Qo;
Q=Qv*Io-Iv*Qo;
DPhi=atan2(Q,I)/M_PI;

/*  update */
DF=K1*DPhi+FreqOsc;
FreqOsc+=K2*DPhi;

PhaseOsc+=DF;
if (PhaseOsc > 1.0) PhaseOsc-=2.0;
if (PhaseOsc <= -1.0) PhaseOsc+=2.0;

return (float)I;
}

static double fr=4800.0/Fe;
static double offset=0.0;

getamp(float *ambuff,int nb)
{
#define BLKIN 4096
static float inbuff[BLKIN];
static int nin=0;
static int idxin=0;

int	n;

	for(n=0;n<nb;n++) {
		float I,Q;

		if(nin<IQFilterLen) {
			int res;

			memmove(inbuff,&(inbuff[idxin]),nin*sizeof(float));
			idxin=0;
			res=getsample(&(inbuff[nin]),BLKIN-nin);
			nin+=res;
			if(nin<IQFilterLen) return(n);
		}

		iqfir(&(inbuff[idxin]),Icoeff,Qcoeff,IQFilterLen,&I,&Q);
		ambuff[n]=pll(I,Q);
		fr=0.25*FreqOsc+0.75*fr;

		idxin++;nin--;
	}
	return(nb);
}

int getpixelv(float *pvbuff,int nb)
{
#define BLKAMP 256
static float ambuff[BLKAMP];
static int nam=0;
static int idxam=0;

int	n;

	for(n=0;n<nb;n++) {
		double mult;
		int shift;

		if(nam<15) {
			int res;

			memmove(ambuff,&(ambuff[idxam]),nam*sizeof(float));
			idxam=0;
			res=getamp(&(ambuff[nam]),BLKAMP-nam);
			nam+=res;
			if(nam<15) return(n);
		}

		mult=(double)Fi/4800.0*fr*FreqLine;
		pvbuff[n]=rsfir(&(ambuff[idxam]),rsfilter,RSFilterLen,offset,mult)*RSMULT*128.0;

		shift=(int)((RSMULT-offset+mult-1)/mult);
		offset=shift*mult+offset-RSMULT;

		idxam+=shift;nam-=shift;
	}
	return(nb);
}

int getpixelrow(float *pixelv)
{
static float pixels[PixelLine+SyncFilterLen];
static int npv=0;
static int synced=0;
static double max=0.0;
float corr,qcorr;
double ph;
int n,res;

   if(npv>0)
	memmove(pixelv,pixels,npv*sizeof(float));

   if(npv<SyncFilterLen) {
	res=getpixelv(&(pixelv[npv]),SyncFilterLen-npv);
	npv+=res;
   	if(npv<SyncFilterLen)
		return(0);
   }

   iqfir(pixelv,ISync,QSync,SyncFilterLen,&corr,&qcorr);
   ph=atan2((double)qcorr,(double)corr)/M_PI*2.0;
   max=corr;
   FreqLine=(double)(PixelLine-SyncFilterLen-ph)/(double)(PixelLine-SyncFilterLen);
   if(corr < 0.75*max) {
	synced=0;
	FreqLine=1.0;
   }
   if(ph >=1.0 || ph <= -1.0)  {
	synced=0;
	FreqLine=1.0;
   }

   if(synced<8) {
	int shift,mshift;

	if(npv<PixelLine+SyncFilterLen) {
		res=getpixelv(&(pixelv[npv]),PixelLine+SyncFilterLen-npv);
		npv+=res;
		if(npv<PixelLine+SyncFilterLen) 
			return(0);
	}

	mshift=0;
	for(shift=1;shift<PixelLine;shift++) {
		double corr;

		corr=fir(&(pixelv[shift]),ISync,SyncFilterLen);
		if(corr>max) {
			mshift=shift;
			max=corr;
		}
	}
	if(mshift !=0) {
		memmove(pixelv,&(pixelv[mshift]),(npv-mshift)*sizeof(float));
		npv-=mshift;
		synced=0;
		FreqLine=1.0;
	} else
		synced+=1;
    }

    if(npv<PixelLine) {
		res=getpixelv(&(pixelv[npv]),PixelLine-npv);
		npv+=res;
    		if(npv<PixelLine) return(0);
    }
    if(npv==PixelLine) {
		npv=0;
    } else  {
		memmove(pixels,&(pixelv[PixelLine]),(npv-PixelLine)*sizeof(float));
		npv-=PixelLine;
   }

    return (1);
}

