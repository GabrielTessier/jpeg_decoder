#include <stdint.h>
#include "ycc2rgb.h"

rgb_t* ycc2rgb(uint8_t y[64], uint8_t cb[64], uint8_t cr[64]) {
  rgb_t* rgb = malloc(sizeof(
}
