/*
 *  Atpdec 
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "filter.h"
#include "filtercoeff.h"

extern int nbch,nch;
extern int getsample(short *inbuff, int nb);

#define Fc 2400.0
#define PixelLine 2080
#define RSMULT 30
#define Fi (PixelLine*2*RSMULT)

double Fe;

static double FreqOsc;
static double DPLL_C1,DPLL_C2,AGCK;
static double FreqLine = 1.0;

#define sig 20

#define RSFilterLen (5*sig+1)
static float rsfilter[RSFilterLen];

int init_dsp(void)
{
int i;

/* gausian filter coeff */

for(i=0;i<RSFilterLen;i++) {
	double x=i-RSFilterLen/2;
	rsfilter[i]=exp(-x*x/2.0/sig/sig)/sqrt(2.0*M_PI)/sig;
}

/* pll coeff */
FreqOsc=Fc/Fe;
DPLL_C2=M_SQRT2*20.0/Fe;
DPLL_C1=DPLL_C2*DPLL_C2/2.0;

AGCK=exp(-1.0/Fe/0.25);
return(0);
}

static int getamp(double *ambuff, int nb)
{

#define BLKIN 4096
    static short inbuff[2*BLKIN];
    int n,ns;

    static double PhaseOsc = 0.0;
    static double mamp=0.5;
    double amp;
    double DPhi;
#define DFc 50.0

    ns=getsample(inbuff, nb > BLKIN ? BLKIN : nb);

    for (n = 0; n < ns; n++) {
	double in;

    in=((double)inbuff[n*nbch+nch]/32768.0);

    mamp=AGCK*mamp+(1.0-AGCK)*fabs(in);
	
    amp=2.0*in*cos(PhaseOsc);
    ambuff[n] = amp;

    //fprintf(stderr,"%g %g %g\n",in,cos(PhaseOsc),amp);

/* delta phase */ 
    DPhi=-in/(mamp*M_SQRT2)*(sin(PhaseOsc)-sin(3.0*PhaseOsc));

/*  loop filter  */

    PhaseOsc += 2.0 * M_PI * (DPLL_C2 * DPhi + FreqOsc);
    if (PhaseOsc > M_PI)
	PhaseOsc -= 2.0 * M_PI;
    if (PhaseOsc <= -M_PI)
	PhaseOsc += 2.0 * M_PI;

    FreqOsc += DPLL_C1 * DPhi;
    if (FreqOsc > ((Fc + DFc) / Fe))
	FreqOsc = (Fc + DFc) / Fe;
    if (FreqOsc < ((Fc - DFc) / Fe))
	FreqOsc = (Fc - DFc) / Fe;


    }
    return (ns);
}

int getpixelv(float *pvbuff, int nb)
{

   static double offset = 0.0;

#define BLKAMP 4096
    static double ambuff[BLKAMP];
    static int nam = 0;
    static int idxam = 0;

    int n,m;
    double mult;
    float v;
    static float ppv=0,pv=0;

    mult = (double) Fi/Fe*FreqLine;

    m=RSFilterLen/mult+1;

    for (n = 0; n < nb; n++) {
	int shift;

	if (nam < m) {
	    int res;
	    memmove(ambuff, &(ambuff[idxam]), nam * sizeof(double));
	    idxam = 0;
	    res = getamp(&(ambuff[nam]), BLKAMP - nam);
	    nam += res;
	    if (nam < m)
		return (n);
	}

	v = rsfir(&(ambuff[idxam]), rsfilter, RSFilterLen, offset, mult) * mult * 256.0;
	if(v<0.0) {
		v = 2.0*(pv-ppv);
		if (v<0.0) v=0.0;
		if (v>=256.0) v=255.0;
		pvbuff[n] = v;
		ppv=pv;
	} else {
		if (v>=256.0) v=255.0;
		pvbuff[n] = v;
		ppv=pv;pv=v;
	}

	shift = ((int) floor((RSMULT - offset) / mult))+1;
	offset = shift*mult+offset-RSMULT ;

	idxam += shift;
	nam -= shift;
    }
    return (nb);
}

int getpixelrow(float *pixelv)
{
    static float pixels[PixelLine + SyncFilterLen];
    static int npv = 0;
    static int synced = 0;
    static double max = 0.0;

    double corr, ecorr, lcorr;
    int res;

    if (npv > 0)
	memmove(pixelv, pixels, npv * sizeof(float));
    if (npv < SyncFilterLen + 2) {
	res = getpixelv(&(pixelv[npv]), SyncFilterLen + 2 - npv);
	npv += res;
	if (npv < SyncFilterLen + 2)
	    return (0);
    }

    /* test sync */
    ecorr = fir(pixelv, Sync, SyncFilterLen);
    corr = fir(&(pixelv[1]), Sync, SyncFilterLen);
    lcorr = fir(&(pixelv[2]), Sync, SyncFilterLen);
    FreqLine = 1.0+((ecorr-lcorr) / corr / PixelLine / 4.0);
    if (corr < 0.75 * max) {
	synced = 0;
	FreqLine = 1.0;
    }
    max = corr;
    if (synced < 8) {
	int shift, mshift;

	if (npv < PixelLine + SyncFilterLen) {
	    res =
		getpixelv(&(pixelv[npv]), PixelLine + SyncFilterLen - npv);
	    npv += res;
	    if (npv < PixelLine + SyncFilterLen)
		return (0);
	}

	/* lookup sync start */
	mshift = 0;
	for (shift = 1; shift < PixelLine; shift++) {
	    double corr;

	    corr = fir(&(pixelv[shift + 1]), Sync, SyncFilterLen);
	    if (corr > max) {
		mshift = shift;
		max = corr;
	    }
	}
	if (mshift != 0) {
	    memmove(pixelv, &(pixelv[mshift]),
		    (npv - mshift) * sizeof(float));
	    npv -= mshift;
	    synced = 0;
	    FreqLine = 1.0;
	} else
	    synced += 1;
    }
    if (npv < PixelLine) {
	res = getpixelv(&(pixelv[npv]), PixelLine - npv);
	npv += res;
	if (npv < PixelLine)
	    return (0);
    }
    if (npv == PixelLine) {
	npv = 0;
    } else {
	memmove(pixels, &(pixelv[PixelLine]),
		(npv - PixelLine) * sizeof(float));
	npv -= PixelLine;
    }

    return (1);
}
