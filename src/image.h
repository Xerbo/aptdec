#include "common.h"

void histogramEqualise(float **prow, int nrow, int offset, int width);
void linearEnhance(float **prow, int nrow, int offset, int width);
int calibrate(float **prow, int nrow, int offset, int width) ;
void denoise(float **prow, int nrow, int offset, int width);
void flipImage(image_t *img, int width, int offset);
int cropNoise(image_t *img);
void temperature(options_t *opts, image_t *img, int offset, int width);
