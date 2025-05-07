#pragma once

#include <stdint.h>
#include "jpeg2ppm.h"

bloctu8_t *idct(bloct16_t *mcu, float ****stockage_coef);
float **calc_cos();
float ****calc_coef();
