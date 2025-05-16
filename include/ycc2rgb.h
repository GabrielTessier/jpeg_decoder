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

// Retourne dans <rgb> les composantes RGB correspondant à <y>,<cb>,<cr>
void ycc2rgb_pixel(uint8_t y, uint8_t cb, uint8_t cr, rgb_t *rgb);
