#pragma once
#include <stdint.h>
#include "jpeg2ppm.h"

struct bloct_s {
  uint8_t data[64];
};
typedef struct bloct_s bloct_t;

struct qtable_s {
  uint8_t data[64];
};
typedef struct qtable_s qtable_t;

bloct_t *iqzz(blocl_t* bloc, qtable_t *qtable);
