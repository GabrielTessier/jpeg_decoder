#pragma once

#include <stdint.h>

struct ycc_s {
  uint8_t y, cb, cr;
};
typedef struct ycc_s ycc_t;

struct mcuycc_s {
  ycc_t mcu[64];
};
typedef struct mcuycc_s mcuycc_t;

struct rgb_s {
  uint8_t r, g, b;
};
typedef struct rgb_s rgb_t;

