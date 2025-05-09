#pragma once

#include <stdint.h>
#include "jpeg2ppm.h"

void Loeffler_iX(float *i0, float *i1);

void Loeffler_iC(float *i0, float *i1, float k, float n);

void Loeffler_iO(float *i0);

float *idct_opt_1D(int16_t mcu[8]);

bloctu8_t *idct_opt(bloct16_t *mcu);
