#include "common.h"

#define MCOMPOSITE(m1, a1, m2, a2) (m1*a1 + m2*a2*(1-a1))

rgb_t applyPalette(char *palette, int val);
rgb_t RGBcomposite(rgb_t top, float top_a, rgb_t bottom, float bottom_a);

char TempPalette[256*3];
char PrecipPalette[58*3];
