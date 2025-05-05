#pragma once
#include <stdint.h>
#include "ycc2rgb.h"

struct mcudct_s {
  int8_t data[8][8];
};
typedef struct mcudct_s mcudct_t;

mcuycc_t idct(mcudct_t *mcu);
