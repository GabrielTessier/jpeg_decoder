#pragma once

#include <stdint.h>
#include "jpeg2ppm.h"

void Loeffler_iX(float *i0, float *i1);

void Loeffler_iC(float *i0, float *i1, float k, float n);

void Loeffler_iO(float *i0);

void reorder(float coefs[8]);

void idct_opt_1D(float coefs[8]);

void transpose(float **mat);

bloctu8_t *idct_opt(bloct16_t *mcu);
