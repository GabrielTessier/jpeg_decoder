#pragma once

#include <stdint.h>


// Structure représentant les trois composantes RGB d'un pixel
struct rgb_s {
  uint8_t r, g, b;
};
typedef struct rgb_s rgb_t;

// Retourne dans <rgb> les composantes RGB correspondant à <y>,<cb>,<cr>
void ycc2rgb_pixel(uint8_t y, uint8_t cb, uint8_t cr, rgb_t *rgb);
