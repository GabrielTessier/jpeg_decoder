#pragma once
#include <stdint.h>
#include "jpeg2ppm.h"

struct mcut_s {
  int8_t data[8][8];
};
typedef struct mcut_s mcut_t;

mcut_t *idct(mcul_t *mcu);
