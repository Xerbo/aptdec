#include "common.h"
#include "offsets.h"
#include "color.h"

int mapOverlay(char *filename, rgb_t **crow, int nrow, int zenith, int MCIR);
int readRawImage(char *filename, float **prow, int *nrow);
int readPalette(char *filename, rgb_t **pixels);
void prow2crow(float **prow, int nrow, char *palette, rgb_t **crow);
int applyUserPalette(float **prow, int nrow, char *filename, rgb_t **crow);
int ImageOut(options_t *opts, image_t *img, int offset, int width, char *desc, char chid, char *palette);
int initWriter(options_t *opts, image_t *img, int width, int height, char *desc, char *chid);
void pushRow(float *row, int width);
void closeWriter();
