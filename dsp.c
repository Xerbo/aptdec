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

static double FreqOsc=2400.0/Fe;
static double FreqLine=1.0;
extern int getsample(float *inbuff ,int nb);

static float pll(float Iv, float Qv)
{
/* pll coeff */
#define K1 5e-3
#define K2 3e-6

static double PhaseOsc=0.0;

double DPhi;
double DF;
double I,Q;
double Io,Qo;

/* quadrature oscillator */
Io=cos(PhaseOsc); Qo=sin(PhaseOsc);

/* phase detector */
I=Iv*Io+Qv*Qo;
Q=Qv*Io-Iv*Qo;
DPhi=atan2(Q,I)/M_PI;

/*  update */
DF=K1*DPhi+FreqOsc;
FreqOsc+=K2*DPhi;

PhaseOsc+=2.0*M_PI*DF;
if (PhaseOsc > M_PI) PhaseOsc-=2.0*M_PI;
if (PhaseOsc <= -M_PI) PhaseOsc+=2.0*M_PI;

return (float)I;
}

static double fr=2400.0/Fe;
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

		mult=(double)Fi*fr/2400.0*FreqLine;
		pvbuff[n]=rsfir(&(ambuff[idxam]),rsfilter,RSFilterLen,offset,mult)*mult*256.0;

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
float corr,scorr;
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

   /* test sync */
   corr=fir(pixelv,Sync,SyncFilterLen);
   scorr=fir(&(pixelv[1]),Sync,SyncFilterLen);
   FreqLine=1.0-scorr/corr/PixelLine/4.0;

   if(corr < 0.75*max) {
	synced=0;
	FreqLine=1.0;
   }
   max=corr;

   if(synced<8) {
	int shift,mshift;

	if(npv<PixelLine+SyncFilterLen) {
		res=getpixelv(&(pixelv[npv]),PixelLine+SyncFilterLen-npv);
		npv+=res;
		if(npv<PixelLine+SyncFilterLen) 
			return(0);
	}

	/* lookup sync start */
	mshift=0;
	for(shift=1;shift<PixelLine;shift++) {
		double corr;

		corr=fir(&(pixelv[shift]),Sync,SyncFilterLen);
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

