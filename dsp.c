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
#ifndef M_PI
#define M_PI 3.14159265358979323846  /* for OS that don't know it */
#endif
#include "filter.h"
#include "filtercoeff.h"

#define Fe 11025.0
#define Fc 2400.0
#define DFc 50.0
#define PixelLine 2080
#define Fp (2*PixelLine)
#define RSMULT 10
#define Fi (Fp*RSMULT)

static double FreqOsc=Fc/Fe;
static double FreqLine=1.0;
extern int getsample(float *inbuff ,int nb);

static float pll(float In)
{
/* pll coeff */
#define K1 5e-3
#define K2 3e-6

static double PhaseOsc=0.0;
static iirbuff_t Ifilterbuff,Qfilterbuff;

double Io,Qo;
double Ip,Qp;
double DPhi;
double DF;

/* quadrature oscillator */
Io=cos(PhaseOsc); Qo=sin(PhaseOsc);

/* phase detector */
Ip=iir(In*Io,&Ifilterbuff,&PhaseFilterCf);
Qp=iir(In*Qo,&Qfilterbuff,&PhaseFilterCf);
DPhi=-atan2(Qp,Ip)/M_PI;

/*  loop filter  */
DF=K1*DPhi+FreqOsc;
FreqOsc+=K2*DPhi;

if(FreqOsc>((Fc+DFc)/Fe)) FreqOsc=(Fc+DFc)/Fe;
if(FreqOsc<((Fc-DFc)/Fe)) FreqOsc=(Fc-DFc)/Fe;

PhaseOsc+=2.0*M_PI*DF;
if (PhaseOsc > M_PI) PhaseOsc-=2.0*M_PI;
if (PhaseOsc <= -M_PI) PhaseOsc+=2.0*M_PI;

return (float)(In*Io);
}

static double fr=Fc/Fe;
static double offset=0.0;

getamp(float *ambuff,int nb)
{
#define BLKIN 1024
float inbuff[BLKIN];
int n;
int res;

  res=getsample(inbuff,nb>BLKIN?BLKIN:nb);

  for(n=0;n<res;n++)  {
    ambuff[n]=pll(inbuff[n]);
    fr=0.25*FreqOsc+0.75*fr;
  }

  return(res);
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

		if(nam<BLKAMP) {
			int res;

			memmove(ambuff,&(ambuff[idxam]),nam*sizeof(float));
			idxam=0;
			res=getamp(&(ambuff[nam]),BLKAMP-nam);
			nam+=res;
			if(nam<BLKAMP) return(n);
		}

		mult=(double)Fi*fr/Fc*FreqLine;
		pvbuff[n]=rsfir(&(ambuff[idxam]),rsfilter,RSFilterLen,offset,mult)*mult*3*256.0;

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
double corr,ecorr,lcorr;
double ph;
int n,res;

   if(npv>0)
	memmove(pixelv,pixels,npv*sizeof(float));

   if(npv<SyncFilterLen+2) {
	res=getpixelv(&(pixelv[npv]),SyncFilterLen+2-npv);
	npv+=res;
   	if(npv<SyncFilterLen+2)
		return(0);
   }

   /* test sync */
   corr=fir(&(pixelv[1]),Sync,SyncFilterLen);
   ecorr=fir(pixelv,Sync,SyncFilterLen);
   lcorr=fir(&(pixelv[2]),Sync,SyncFilterLen);
   FreqLine=1.0+(ecorr-lcorr)/corr/PixelLine/4.0;

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

		corr=fir(&(pixelv[shift+1]),Sync,SyncFilterLen);
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

