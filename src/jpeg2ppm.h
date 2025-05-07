#pragma once

#include <stdint.h>

struct blocl8_s {
  int8_t data[64];
};
typedef struct blocl8_s blocl8_t;

struct bloct8_s {
  int8_t data[8][8];
};
typedef struct bloct8_s bloct8_t;

struct bloclu8_s {
  uint8_t data[64];
};
typedef struct bloclu8_s bloclu8_t;

struct bloctu8_s {
  uint8_t data[8][8];
};
typedef struct bloctu8_s bloctu8_t;

struct blocl16_s {
  int16_t data[64];
};
typedef struct blocl16_s blocl16_t;

struct bloct16_s {
  int16_t data[8][8];
};
typedef struct bloct16_s bloct16_t;
