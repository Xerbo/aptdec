#include "apt.h"
#include "common.h"

int readRawImage(char *filename, float **prow, int *nrow);
int readPalette(char *filename, apt_rgb_t **pixels);
void prow2crow(float **prow, int nrow, char *palette, apt_rgb_t **crow);
int applyUserPalette(float **prow, int nrow, char *filename, apt_rgb_t **crow);
int ImageOut(options_t *opts, apt_image_t *img, int offset, int width, char *desc, char chid, char *palette);
int initWriter(options_t *opts, apt_image_t *img, int width, int height, char *desc, char *chid);
void pushRow(float *row, int width);
void closeWriter();
