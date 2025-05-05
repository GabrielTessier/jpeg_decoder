#pragma once
#include <stdint.h>
#include "idct.h"

struct mcuq_s {
  uint8_t data[64];
};
typedef struct mcuq_s mcuq_t;

struct qtable_s {
  uint8_t data[64];
};
typedef struct qtable_s qtable_t;

mcudct_t *iqzz(mcuq_t* bloc, qtable_t *qtable);
