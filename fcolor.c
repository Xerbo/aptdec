#include <stdio.h>
#include <math.h>


typedef struct {
float h,s,v;
} hsvpix_t;

static void HSVtoRGB( float *r, float *g, float *b, hsvpix_t pix )
{
    int i;
    double f, p, q, t, h;

    if( pix.s == 0 ) {
        // achromatic (grey)
        *r = *g = *b = pix.v;
        return;
    }

    h = pix.h/60;            // sector 0 to 5
    i = floor( h );
    f = h - i;            // factorial part of h
    p = pix.v * ( 1 - pix.s );
    q = pix.v * ( 1 - pix.s * f );
    t = pix.v * ( 1 - pix.s * ( 1 - f ) );

    switch( i ) {
        case 0:
            *r = pix.v; *g = t; *b = p;
            break;
        case 1:
            *r = q; *g = pix.v; *b = p;
            break;
        case 2:
            *r = p; *g = pix.v; *b = t;
            break;
        case 3:
            *r = p; *g = q; *b = pix.v;
            break;
        case 4:
            *r = t; *g = p; *b = pix.v;
            break;
        default:        // case 5:
            *r = pix.v; *g = p; *b = q;
            break;
    }

}

static struct  {
float Vthresold;
float Tthresold;
hsvpix_t CloudTop;
hsvpix_t CloudBot;
hsvpix_t SeaTop;
hsvpix_t SeaBot;
hsvpix_t GroundTop;
hsvpix_t GroundBot;
} fcinfo= {
	30.0,	
	170.0,
	{0,0.0,0.3},{0,0.0,1.0},
	{240.0,0.9,0.5},{220.0,0.8,0.8},
	{90.0,0.8,0.3},{50.0,0.8,1.0}
};

void readfconf(char *file)
{
FILE * fin;

fin=fopen(file,"r");
if(fin==NULL) return;

fscanf(fin,"%g\n",&fcinfo.Vthresold);
fscanf(fin,"%g\n",&fcinfo.Tthresold);
fscanf(fin,"%g %g %g\n",&fcinfo.CloudTop.h,&fcinfo.CloudTop.s,&fcinfo.CloudTop.v);
fscanf(fin,"%g %g %g\n",&fcinfo.CloudBot.h,&fcinfo.CloudBot.s,&fcinfo.CloudBot.v);
fscanf(fin,"%g %g %g\n",&fcinfo.SeaTop.h,&fcinfo.SeaTop.s,&fcinfo.SeaTop.v);
fscanf(fin,"%g %g %g\n",&fcinfo.SeaBot.h,&fcinfo.SeaBot.s,&fcinfo.SeaBot.v);
fscanf(fin,"%g %g %g\n",&fcinfo.GroundTop.h,&fcinfo.GroundTop.s,&fcinfo.GroundTop.v);
fscanf(fin,"%g %g %g\n",&fcinfo.GroundBot.h,&fcinfo.GroundBot.s,&fcinfo.GroundBot.v);

fclose(fin);

};

void falsecolor(double v, double t, float *r, float *g, float *b)
{
hsvpix_t top,bot,c;
double scv,sct;

if(v <= fcinfo.Vthresold) {
	/* sea */
	top=fcinfo.SeaTop,bot=fcinfo.SeaBot;
	scv=v/fcinfo.Vthresold;
	sct=t/255.0;
} else  {
	if(t<=fcinfo.Tthresold) {
		/* clouds */
		top=fcinfo.CloudTop,bot=fcinfo.CloudBot;
		scv=(v-fcinfo.Vthresold)/(255.0-fcinfo.Vthresold);
		sct=t/fcinfo.Tthresold;
	} else {
		/* ground */
		top=fcinfo.GroundTop,bot=fcinfo.GroundBot;
		scv=(v-fcinfo.Vthresold)/(255.0-fcinfo.Vthresold);
		sct=(t-fcinfo.Tthresold)/(255.0-fcinfo.Tthresold);
	}
};

c.h=top.h+sct*(bot.h-top.h);
c.v=top.v+scv*(bot.v-top.v);
c.s=top.s+scv*sct*(bot.s-top.s);

HSVtoRGB( r, g, b, c );
};





