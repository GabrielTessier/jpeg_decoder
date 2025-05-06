#pragma once

#include <stdint.h>
#include "jpeg2ppm.h"

struct rgb_s {
  uint8_t r, g, b;
};
typedef struct rgb_s rgb_t;

struct bloc_rgb_s {
  rgb_t data[8][8];
};
typedef struct bloc_rgb_s bloc_rgb_t;

bloc_rgb_t *ycc2rgb(bloct_t y[64], bloct_t cb[64], bloct_t cr[64]);
