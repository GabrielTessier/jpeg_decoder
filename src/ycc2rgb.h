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

struct mcurgb_s {
  rgb_t data[8][8];
};
typedef struct mcurgb_s mcurgb_t;

mcurgb_t* ycc2rgb(mcuycc_t y[64], mcuycc_t cb[64], mcuycc_t cr[64]);
