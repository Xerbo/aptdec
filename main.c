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
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <sndfile.h>
#include <png.h>
#include <cmap.h>

extern getpixelrow(float *pixelv);


static SNDFILE *inwav;
static int initsnd(char *filename)
{
SF_INFO infwav;

/* open wav input file */
infwav.format=0;
inwav=sf_open(filename,SFM_READ,&infwav);
if (inwav==NULL) {
	fprintf(stderr,"could not open %s\n",filename);
	return(1);
}
if(infwav.samplerate !=11025) {
	fprintf(stderr,"Bad Input File sample rate: %d. Must be 11025\n",infwav.samplerate);
	return(1);
}
if(infwav.channels !=1) {
	fprintf(stderr,"Too many channels in input file : %d\n",infwav.channels);
	return(1);
}

return(0);
}

int getsample(float *sample,int nb)
{
return(sf_read_float(inwav,sample,nb));
}

png_text text_ptr[2]={
{ PNG_TEXT_COMPRESSION_NONE, "Software", "Created by Thierry Leconte's atpdec",35 }
};
static int ImageOut(char *filename,float **prow,int nrow,int width,int offset)
{
FILE *pngfile;
png_infop info_ptr;
png_structp png_ptr;
int n;

pngfile=fopen(filename,"w");
if (pngfile==NULL) {
	fprintf(stderr,"could not open %s\n",filename);
	return(1);
}
/* init png lib */
png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
if (!png_ptr) {
	fprintf(stderr,"could not open create png_ptr\n");
	return(1);
}

info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
	fprintf(stderr,"could not open create info_ptr\n");
	return(1);
    }

png_init_io(png_ptr,pngfile);
png_set_IHDR(png_ptr, info_ptr, width, nrow,
       8, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE,
       PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

png_set_text(png_ptr, info_ptr, text_ptr, 1);

printf("Writing %s\n",filename);
png_write_info(png_ptr,info_ptr);
for(n=0;n<nrow;n++) {
	float *pixelv;
	png_byte pixel[2080];
	int	i;

	pixelv=prow[n];
	for(i=0;i<width;i++) {
		float pv;

		pv=pixelv[i+offset];
		pixel[i]=(png_byte)(pv*255.0);
	}
	png_write_row(png_ptr,pixel);
}
png_write_end(png_ptr,info_ptr);
fclose(pngfile);
png_destroy_write_struct(&png_ptr,&info_ptr);
return(0);
} 

int ImageColorOut(char *filename,float **prow,int nrow)
{
FILE *pngfile;
png_infop info_ptr;
png_structp png_ptr;
int n;

pngfile=fopen(filename,"w");
if (pngfile==NULL) {
	fprintf(stderr,"could not open %s\n",filename);
	return(1);
}
/* init png lib */
png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
if (!png_ptr) {
	fprintf(stderr,"could not open create png_ptr\n");
	return(1);
}

info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
	fprintf(stderr,"could not open create info_ptr\n");
	return(1);
    }

png_init_io(png_ptr,pngfile);
png_set_IHDR(png_ptr, info_ptr, 909, nrow,
       8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
       PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

png_set_text(png_ptr, info_ptr, text_ptr, 1);

printf("Writing %s\n",filename);
png_write_info(png_ptr,info_ptr);

for(n=0;n<nrow;n++) {
	float *pixelv;
	png_color pixel[909];
	int	i;

	pixelv=prow[n];
	for(i=0;i<909;i++) {
		int x,y;

		y=(int)(pixelv[i+85]*255.0);
		x=(int)(pixelv[i+1125]*255.0);
		pixel[i]=cmap[y][x];
	}
	png_write_row(png_ptr,(png_bytep)pixel);
}
png_write_end(png_ptr,info_ptr);
fclose(pngfile);
png_destroy_write_struct(&png_ptr,&info_ptr);
return(0);
} 

readcmap(char *filename)
{
FILE *pngfile;
png_infop info_ptr;
png_structp png_ptr;
int n;
png_bytep prow[256];


pngfile=fopen(filename,"r");
if (pngfile==NULL) {
	fprintf(stderr,"could not open %s\n",filename);
	return(1);
}
/* init png lib */
png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
if (!png_ptr) {
	fprintf(stderr,"could not open create png_ptr\n");
	return(1);
}

info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
	fprintf(stderr,"could not open create info_ptr\n");
	return(1);
    }
png_init_io(png_ptr,pngfile);

for(n=0;n<256;n++)
	prow[n]=(png_bytep)&(cmap[n][0]);

png_read_info(png_ptr, info_ptr);

if(png_get_image_width(png_ptr,info_ptr)!=256) {
	fprintf(stderr,"Invalid cmap width\n");
	exit(1);
}
if(png_get_image_height(png_ptr,info_ptr)!=256) {
	fprintf(stderr,"Invalid cmap height\n");
	exit(1);
}
if(png_get_bit_depth(png_ptr,info_ptr)!=8) {
	fprintf(stderr,"Invalid cmap depth\n");
	exit(1);
}
if(png_get_color_type(png_ptr,info_ptr)!=PNG_COLOR_TYPE_RGB  ){
	fprintf(stderr,"Invalid cmap color type\n");
	exit(1);
}

png_read_image(png_ptr,prow);
fclose(pngfile);
png_destroy_read_struct(&png_ptr,&info_ptr,NULL);
return(0);
}

#ifdef DEBUG
unsigned int distrib[256][256];
int Distrib(char *filename,float **prow,int nrow)
{
int n;
int x,y;
int max=0;
FILE *df;

for(y=0;y<256;y++)
	for(x=0;x<256;x++)
                distrib[y][x]=0;

for(n=0;n<nrow;n++) {
        float *pixelv;
        png_color pixel[909];
        int     i;

        pixelv=prow[n];
        for(i=0;i<909;i++) {

                y=(int)(pixelv[i+85]*255.0);
                x=(int)(pixelv[i+1125]*255.0);
		if(x>255) x=255;
		if(x<0) x=0;
		if(y>255) y=255;
		if(y<0) y=0;
                distrib[y][x]+=1;
		if(distrib[y][x]> max) max=distrib[y][x];
        }
}
df=fopen(filename,"w");
fprintf(df,"P2\n#max %d\n",max);
fprintf(df,"256 256\n255\n");
for(y=0;y<256;y++)
	for(x=0;x<256;x++)
		fprintf(df,"%d\n",(int)((255.0*(double)(distrib[y][x]))/(double)max));
fclose(df);
}
#endif

extern int Calibrate(float **prow,int nrow,int offset);
extern int optind,opterr;
extern char *optarg;

main(int argc, char **argv)
{
char pngfilename[1024];
char name[1024];
char *pngdirname=NULL;
char imgopt[20]="abc";
float *prow[3000];
const char *chid[6]={ "1","2","3A","4","5","3B"};
int n,nrow;
int ch;
int c;


opterr=0;
while ((c=getopt(argc,argv,"c:d:i:"))!=EOF) {
	switch(c) {
		case 'd':
			pngdirname=optarg;
			break;
		case 'c':
			readcmap(optarg);	
			break;
		case 'i':
			strcpy(imgopt,optarg);
			break;
	}
}

for(nrow=0;nrow<3000;nrow++) prow[nrow]=NULL;

for (;optind<argc;optind++) {
int a=0,b=0;

strcpy(pngfilename,argv[optind]);
strcpy(name,basename(pngfilename));
strtok(name,".");
if (pngdirname==NULL) {
	strcpy(pngfilename,argv[optind]);
	pngdirname=dirname(pngfilename);
}

/* open snd input */
if(initsnd(argv[optind]))
	exit(1);

/* main loop */
printf("Decoding: %s ...",argv[optind]);
fflush(stdout);
for(nrow=0;nrow<3000;nrow++) {
	if(prow[nrow]==NULL) prow[nrow]=(float*)malloc(sizeof(float)*2150);
	if(getpixelrow(prow[nrow])==0)
		break;
} 
printf("Done : %d lines\n",nrow); 
sf_close(inwav);

/* raw image */
if(strchr(imgopt,(int)'r')!=NULL){
sprintf(pngfilename,"%s/%s.png",pngdirname,name);
ImageOut(pngfilename,prow,nrow,2080,0);
}

/* Channel A */
if(((strchr(imgopt,(int)'a')!=NULL) || (strchr(imgopt,(int)'c')!=NULL))) {
	ch=Calibrate(prow,nrow,85);
	if(ch>0) {
		a=1;
		if(strchr(imgopt,(int)'a')!=NULL) {
		sprintf(pngfilename,"%s/%s-%s.png",pngdirname,name,chid[ch]);
		ImageOut(pngfilename,prow,nrow,954,85);
		}
	}
}

/* Channel B */
if(((strchr(imgopt,(int)'b')!=NULL) || (strchr(imgopt,(int)'c')!=NULL))) {
	ch=Calibrate(prow,nrow,1125);
	if(ch>0) {
		b=1;
		if(strchr(imgopt,(int)'b')!=NULL) {
		sprintf(pngfilename,"%s/%s-%s.png",pngdirname,name,chid[ch]);
		ImageOut(pngfilename,prow,nrow,954,1125);
		}
	}
}

#ifdef DEBUG
	sprintf(pngfilename,"%s/%s-d.pnm",pngdirname,name);
	Distrib(pngfilename,prow,nrow);
#endif

/* color image */
if(a && b && strchr(imgopt,(int)'c')!=NULL){
	sprintf(pngfilename,"%s/%s-c.png",pngdirname,name);
	ImageColorOut(pngfilename,prow,nrow);
}

}
exit (0);
}
