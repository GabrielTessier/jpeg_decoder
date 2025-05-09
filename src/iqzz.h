#pragma once

#include <stdint.h>
#include "jpeg2ppm.h"

struct qtable_s {
  uint8_t data[64];
};
typedef struct qtable_s qtable_t;

blocl16_t *iquant(blocl16_t *entree, qtable_t *qtable);
bloct16_t *izz(blocl16_t *entree);
