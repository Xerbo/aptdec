#include <stdio.h>
#include <math.h>


typedef struct
{
  float h, s, v;
} hsvpix_t;

static void
HSVtoRGB (float *r, float *g, float *b, hsvpix_t pix)
{
  int i;
  double f, p, q, t, h;

  if (pix.s == 0) {
    // achromatic (grey)
    *r = *g = *b = pix.v;
    return;
  }

  h = pix.h / 60;		// sector 0 to 5
  i = floor (h);
  f = h - i;			// factorial part of h
  p = pix.v * (1 - pix.s);
  q = pix.v * (1 - pix.s * f);
  t = pix.v * (1 - pix.s * (1 - f));

  switch (i) {
  case 0:
    *r = pix.v;
    *g = t;
    *b = p;
    break;
  case 1:
    *r = q;
    *g = pix.v;
    *b = p;
    break;
  case 2:
    *r = p;
    *g = pix.v;
    *b = t;
    break;
  case 3:
    *r = p;
    *g = q;
    *b = pix.v;
    break;
  case 4:
    *r = t;
    *g = p;
    *b = pix.v;
    break;
  default:			// case 5:
    *r = pix.v;
    *g = p;
    *b = q;
    break;
  }

}

static struct
{
  float Seathresold;
  float Landthresold;
  float Tthresold;
  hsvpix_t CloudTop;
  hsvpix_t CloudBot;
  hsvpix_t SeaTop;
  hsvpix_t SeaBot;
  hsvpix_t GroundTop;
  hsvpix_t GroundBot;
} fcinfo = {
  30.0, 90.0, 100.0, {
  230, 0.2, 0.3}, {
  230, 0.0, 1.0}, {
  200.0, 0.7, 0.6}, {
  240.0, 0.6, 0.4}, {
  60.0, 0.6, 0.2}, {
  100.0, 0.0, 0.5}
};

void
readfconf (char *file)
{
  FILE *fin;

  fin = fopen (file, "r");
  if (fin == NULL)
    return;

  fscanf (fin, "%g\n", &fcinfo.Seathresold);
  fscanf (fin, "%g\n", &fcinfo.Landthresold);
  fscanf (fin, "%g\n", &fcinfo.Tthresold);
  fscanf (fin, "%g %g %g\n", &fcinfo.CloudTop.h, &fcinfo.CloudTop.s,
	  &fcinfo.CloudTop.v);
  fscanf (fin, "%g %g %g\n", &fcinfo.CloudBot.h, &fcinfo.CloudBot.s,
	  &fcinfo.CloudBot.v);
  fscanf (fin, "%g %g %g\n", &fcinfo.SeaTop.h, &fcinfo.SeaTop.s,
	  &fcinfo.SeaTop.v);
  fscanf (fin, "%g %g %g\n", &fcinfo.SeaBot.h, &fcinfo.SeaBot.s,
	  &fcinfo.SeaBot.v);
  fscanf (fin, "%g %g %g\n", &fcinfo.GroundTop.h, &fcinfo.GroundTop.s,
	  &fcinfo.GroundTop.v);
  fscanf (fin, "%g %g %g\n", &fcinfo.GroundBot.h, &fcinfo.GroundBot.s,
	  &fcinfo.GroundBot.v);

  fclose (fin);

};

void
falsecolor (double v, double t, float *r, float *g, float *b)
{
  hsvpix_t top, bot, c;
  double scv, sct;

  if (t < fcinfo.Tthresold) {
    if (v < fcinfo.Seathresold) {
      /* sea */
      top = fcinfo.SeaTop, bot = fcinfo.SeaBot;
      scv = v / fcinfo.Seathresold;
      sct = t / fcinfo.Tthresold;
    }
    else {
      /* ground */
      top = fcinfo.GroundTop, bot = fcinfo.GroundBot;
      scv =
	(v - fcinfo.Seathresold) / (fcinfo.Landthresold - fcinfo.Seathresold);
      sct = t / fcinfo.Tthresold;
    }
  }
  else {
    /* clouds */
    top = fcinfo.CloudTop, bot = fcinfo.CloudBot;
    scv = v / 255.0;
    sct = t / 255.0;
  }

  c.s = top.s + sct * (bot.s - top.s);
  c.v = top.v + scv * (bot.v - top.v);
  c.h = top.h + scv * sct * (bot.h - top.h);

  HSVtoRGB (r, g, b, c);
};
