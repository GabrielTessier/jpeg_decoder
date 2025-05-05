#pragma once
#include <stdint.h>
#include "idct.h"

struct mcuq_s {
  uint8_t img[64];
};
typedef struct mcuq_s mcuq_t;

mcudct_t *iqzz(mcuq_t* bloc);
