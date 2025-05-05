#include <stdint.h>
#include <stdlib.h>
#include "ycc2rgb.h"

rgb_t* ycc2rgb(uint8_t y[64], uint8_t cb[64], uint8_t cr[64]) {
  rgb_t* rgb = (rgb_t*) malloc(sizeof(rgb_t)*64);
  for (int i=0; i<64; i++) {
    rgb[i].r = y[i] - 0.0009267*(cb[i]-128) + 1.4016868*(cr[i]-128);
    rgb[i].g = y[i] - 0.3436954*(cb[i]-128) - 0.7141690*(cr[i]-128);
    rgb[i].b = y[i] + 1.7721604*(cb[i]-128) + 0.0009902*(cr[i]-128);
  }
  return rgb;
}
