#pragma once

#include <stdint.h>

struct rgb_s {
  uint8_t r, g, b;
};
typedef struct rgb_s rgb_t;

struct bloc_rgb_s {
  rgb_t data[8][8];
};
typedef struct bloc_rgb_s bloc_rgb_t;

bloc_rgb_t *yc2rgb(bloc_rgb_t y[64], bloc_rgb_t cb[64], bloc_rgb_t cr[64]);
