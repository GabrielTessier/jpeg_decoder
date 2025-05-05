#pragma once
#include <stdint.h>
#include "ycc2rgb.h"

struct mcudct_s {
  int8_t mcu[64];
};
typedef struct mcudct_s mcudct_t;

mcuycc_t idct(mcudct_t mcu);
