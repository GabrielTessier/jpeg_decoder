#pragma once
#include <stdint.h>
#include "jpeg2ppm.h"

struct mcut_s {
  uint8_t data[64];
};
typedef struct mcut_s mcut_t;

struct qtable_s {
  uint8_t data[64];
};
typedef struct qtable_s qtable_t;

mcut_t *iqzz(mcul_t* bloc, qtable_t *qtable);
