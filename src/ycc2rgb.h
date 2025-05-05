#pragma once
#include <stdint.h>

struct mcuycc_s {
  uint8_t data[8][8];
};
typedef struct mcuycc_s mcuycc_t;

struct rgb_s {
  uint8_t r, g, b;
};
typedef struct rgb_s rgb_t;

rgb_t* ycc2rgb(uint8_t y[64], uint8_t cb[64], uint8_t cr[64]);
