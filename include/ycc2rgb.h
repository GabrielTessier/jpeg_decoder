#pragma once

#include <stdint.h>
#include <jpeg2ppm.h>

struct rgb_s {
  uint8_t r, g, b;
};
typedef struct rgb_s rgb_t;

struct bloc_rgb_s {
  rgb_t data[8][8];
};
typedef struct bloc_rgb_s bloc_rgb_t;

bloc_rgb_t *ycc2rgb_bloc(bloctu8_t y[64], bloctu8_t cb[64], bloctu8_t cr[64]);
void ycc2rgb_pixel(uint8_t y, uint8_t cb, uint8_t cr, rgb_t *rgb);
