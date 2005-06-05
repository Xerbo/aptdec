/*
 *  Atpdec
 *  Copyright (c) 2004-2005 by Thierry Leconte (F4DWV)
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
#ifdef WIN32
#include "w32util.h"
#else
#include <libgen.h>
#endif
#include <string.h>
#include <sndfile.h>
#include <png.h>

#include "version.h"

extern int getpixelrow(float *pixelv);
extern int init_dsp(double F);;

#define SYNC_WIDTH 39
#define SPC_WIDTH 47
#define TELE_WIDTH 45
#define CH_WIDTH  909
#define CH_OFFSET  (SYNC_WIDTH+SPC_WIDTH+CH_WIDTH+TELE_WIDTH)
#define IMG_WIDTH  2080

static SNDFILE *inwav;

static int initsnd(char *filename)
{
    SF_INFO infwav;
    int	    res;

/* open wav input file */
    infwav.format = 0;
    inwav = sf_open(filename, SFM_READ, &infwav);
    if (inwav == NULL) {
	fprintf(stderr, "could not open %s\n", filename);
	return (1);
    }

    res=init_dsp(infwav.samplerate);
    if(res<0) {
	fprintf(stderr, "Sample rate too low : %d\n", infwav.samplerate);
	return (1);
    }
    if(res>0) {
	fprintf(stderr, "Sample rate too hight : %d\n", infwav.samplerate);
	return (1);
    }
    fprintf(stderr, "Sample rate : %d\n", infwav.samplerate);

    if (infwav.channels != 1) {
	fprintf(stderr, "Too many channels in input file : %d\n", infwav.channels);
	return (1);
    }

    return (0);
}

int getsample(float *sample, int nb)
{
    return (sf_read_float(inwav, sample, nb));
}

static png_text text_ptr[] = {
    {PNG_TEXT_COMPRESSION_NONE, "Software", version, sizeof(version)}
    ,
    {PNG_TEXT_COMPRESSION_NONE, "Channel", NULL, 0}
    ,
    {PNG_TEXT_COMPRESSION_NONE, "Description", "NOAA POES satellite Image",
     25}
};

static int
ImageOut(char *filename, char *chid, float **prow, int nrow, 
	 int width, int offset)
{
    FILE *pngfile;
    png_infop info_ptr;
    png_structp png_ptr;
    int n;

/* init png lib */
    png_ptr =
	png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
	fprintf(stderr, "could not open create png_ptr\n");
	return (1);
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
	png_destroy_write_struct(&png_ptr, (png_infopp) NULL);
	fprintf(stderr, "could not open create info_ptr\n");
	return (1);
    }

    png_set_IHDR(png_ptr, info_ptr, width, nrow,
		 8, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE,
		 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    text_ptr[1].text = chid;
    text_ptr[1].text_length = strlen(chid);
    png_set_text(png_ptr, info_ptr, text_ptr, 3);
    png_set_pHYs(png_ptr, info_ptr, 4000, 4000, PNG_RESOLUTION_METER);

    printf("Writing %s ... ", filename);
    fflush(stdout);
    pngfile = fopen(filename, "wb");
    if (pngfile == NULL) {
	fprintf(stderr, "could not open %s\n", filename);
	return (1);
    }
    png_init_io(png_ptr, pngfile);
    png_write_info(png_ptr, info_ptr);

    for (n = 0; n < nrow; n++) {
	float *pixelv;
	png_byte pixel[2*IMG_WIDTH];
	int i;

	pixelv = prow[n];
	for (i = 0; i < width; i++) {
	    pixel[i] = pixelv[i + offset];
	}
	png_write_row(png_ptr, pixel);
    }
    png_write_end(png_ptr, info_ptr);
    fclose(pngfile);
    printf("Done\n");
    png_destroy_write_struct(&png_ptr, &info_ptr);
    return (0);
}

int ImageColorOut(char *filename, float **prow, int nrow)
{
    FILE *pngfile;
    png_infop info_ptr;
    png_structp png_ptr;
    int n;
    float *pixelc, *pixelp;

    extern void falsecolor(double v, double t, float *r, float *g,
			    float *b);

/* init png lib */
    png_ptr =
	png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
	fprintf(stderr, "could not open create png_ptr\n");
	return (1);
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
	png_destroy_write_struct(&png_ptr, (png_infopp) NULL);
	fprintf(stderr, "could not open create info_ptr\n");
	return (1);
    }

    png_set_IHDR(png_ptr, info_ptr, CH_WIDTH , nrow ,
		 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
		 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_set_pHYs(png_ptr, info_ptr, 4000, 4000, PNG_RESOLUTION_METER);

    text_ptr[1].text = "False Colors";
    text_ptr[1].text_length = strlen(text_ptr[1].text);
    png_set_text(png_ptr, info_ptr, text_ptr, 3);

    printf("Computing False colors & writing : %s ...", filename);
    fflush(stdout);
    pngfile = fopen(filename, "wb");
    if (pngfile == NULL) {
	fprintf(stderr, "could not open %s\n", filename);
	return (1);
    }
    png_init_io(png_ptr, pngfile);
    png_write_info(png_ptr, info_ptr);

    pixelc=prow[0];
    for (n = 0; n < nrow ; n++) {
	png_color pix[CH_WIDTH];
	int i;

	pixelp=pixelc;
	pixelc = prow[n];

	for (i = 0; i < CH_WIDTH - 1; i++) {
	    float v, t;
  	    float r, g, b;

	    v = pixelc[i+SYNC_WIDTH + SPC_WIDTH];
	    t = (2.0*pixelc[i+SYNC_WIDTH + SPC_WIDTH + CH_OFFSET]+pixelp[i+SYNC_WIDTH + SPC_WIDTH + CH_OFFSET])/3.0;

	    falsecolor(v, t, &r, &g, &b);

	    pix[i].red = 255.0 * r; 
	    pix[i].green = 255.0 * g;
	    pix[i].blue = 255.0 * b;
	}
	png_write_row(png_ptr, (png_bytep) pix);
    }
    png_write_end(png_ptr, info_ptr);
    fclose(pngfile);
    printf("Done\n");
    png_destroy_write_struct(&png_ptr, &info_ptr);
    return (0);
}

extern int Calibrate(float **prow, int nrow, int offset);
extern void Temperature(float **prow, int nrow, int ch, int offset);
extern void readfconf(char *file);
extern int optind, opterr;
extern char *optarg;
int satnum = 1;

static void usage(void)
{
    fprintf(stderr, "atpdec [options] soundfiles ...\n");
    fprintf(stderr,
	    "options:\n-d <dir>\tDestination directory\n-i [r|a|b|c|t]\tOutput image type\n\t\t\tr: Raw\n\t\t\ta: A chan.\n\t\t\tb: B chan.\n\t\t\tc: False color\n\t\t\tt: Temperature\n-c <file>\tFalse color config file\n-s [15|16|17|18]\tSatellite number (for temperature and false color generation)\n");
   exit(1);
}

int main(int argc, char **argv)
{
    char pngfilename[1024];
    char name[1024];
    char pngdirname[1024] = "";
    char imgopt[20] = "ac";
    float *prow[3000];
    char *chid[6] = { "1", "2", "3A", "4", "5", "3B" };
    int nrow;
    int ch;
    int c;

    printf("%s\n", version);

    opterr = 0;
    while ((c = getopt(argc, argv, "c:d:i:s:")) != EOF) {
	switch (c) {
	case 'd':
	    strcpy(pngdirname, optarg);
	    break;
	case 'c':
	    readfconf(optarg);
	    break;
	case 'i':
	    strcpy(imgopt, optarg);
	    break;
	case 's':
	    satnum = atoi(optarg)-15;
	    if (satnum < 0 || satnum > 3) {
		fprintf(stderr, "invalid satellite number  : must be in [15-18]\n");
		exit(1);
	    }
	    break;
	default:
	    usage();
	}
    }

    for (nrow = 0; nrow < 3000; nrow++)
	prow[nrow] = NULL;

    for (; optind < argc; optind++) {
	int a = 0, b = 0;

	strcpy(pngfilename, argv[optind]);
	strcpy(name, basename(pngfilename));
	strtok(name, ".");
	if (pngdirname[0] == '\0') {
	    strcpy(pngfilename, argv[optind]);
	    strcpy(pngdirname, dirname(pngfilename));
	}

/* open snd input */
	if (initsnd(argv[optind]))
	    exit(1);

/* main loop */
	printf("Decoding: %s \n", argv[optind]);
	for (nrow = 0; nrow < 3000; nrow++) {
	    if (prow[nrow] == NULL)
		prow[nrow] = (float *) malloc(sizeof(float) * 2150);
	    if (getpixelrow(prow[nrow]) == 0)
		break;
	    printf("%d\r", nrow);
	    fflush(stdout);
	}
	printf("\nDone\n");
	sf_close(inwav);

/* raw image */
	if (strchr(imgopt, (int) 'r') != NULL) {
	    sprintf(pngfilename, "%s/%s-r.png", pngdirname, name);
	    ImageOut(pngfilename, "raw", prow, nrow, IMG_WIDTH, 0);
	}

/* Channel A */
	if (((strchr(imgopt, (int) 'a') != NULL)
	     || (strchr(imgopt, (int) 'c') != NULL)
	     || (strchr(imgopt, (int) 'd') != NULL))) {
	    ch = Calibrate(prow, nrow, SYNC_WIDTH);
	    if (ch >= 0) {
		if (strchr(imgopt, (int) 'a') != NULL) {
		    sprintf(pngfilename, "%s/%s-%s.png", pngdirname, name,
			    chid[ch]);
		    ImageOut(pngfilename, chid[ch], prow, nrow, 
			     SPC_WIDTH + CH_WIDTH + TELE_WIDTH,
			     SYNC_WIDTH);
		}
	    }
	    if (ch < 2)
		a = 1;
	}

/* Channel B */
	if ((strchr(imgopt, (int) 'b') != NULL)
	    || (strchr(imgopt, (int) 'c') != NULL)
	    || (strchr(imgopt, (int) 't') != NULL)
	    || (strchr(imgopt, (int) 'd') != NULL)) {
	    ch = Calibrate(prow, nrow, CH_OFFSET + SYNC_WIDTH);
	    if (ch >= 0) {
		if (strchr(imgopt, (int) 'b') != NULL) {
		    sprintf(pngfilename, "%s/%s-%s.png", pngdirname, name,
			    chid[ch]);
		    ImageOut(pngfilename, chid[ch], prow, nrow, 
			     SPC_WIDTH + CH_WIDTH + TELE_WIDTH,
			     CH_OFFSET + SYNC_WIDTH);
		}
	    }
	    if (ch > 2) {
		b = 1;
		Temperature(prow, nrow, ch, CH_OFFSET + SYNC_WIDTH);
		if (strchr(imgopt, (int) 't') != NULL) {
		    sprintf(pngfilename, "%s/%s-t.png", pngdirname, name);
		    ImageOut(pngfilename, "Temperature", prow, nrow,
			     CH_WIDTH, CH_OFFSET + SYNC_WIDTH + SPC_WIDTH);
		}
	    }
	}

/* distribution */
	if (a && b && strchr(imgopt, (int) 'd') != NULL) {
	    sprintf(pngfilename, "%s/%s-d.pnm", pngdirname, name);
	}

/* color image */
	if (a && b && strchr(imgopt, (int) 'c') != NULL) {
	    sprintf(pngfilename, "%s/%s-c.png", pngdirname, name);
	    ImageColorOut(pngfilename, prow, nrow);
	}

    }
    exit(0);
}
