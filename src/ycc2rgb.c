#include <stdint.h>
#include <stdlib.h>
#include "ycc2rgb.h"
#include "jpeg2ppm.h"

bloc_rgb_t *ycc2rgb(bloctu8_t y[64], bloctu8_t cb[64], bloctu8_t cr[64]) {
  bloc_rgb_t *rgb = (bloc_rgb_t*) malloc(sizeof(bloc_rgb_t));
  for (int i=0; i<8; i++) 
    for (int j=0; j<8; j++) {
      rgb->data[i][j].r = y->data[i][j] - 0.0009267*((float)cb->data[i][j]-128) + 1.4016868*((float)cr->data[i][j]-128);
      rgb->data[i][j].g = y->data[i][j] - 0.3436954*((float)cb->data[i][j]-128) - 0.7141690*((float)cr->data[i][j]-128);
      rgb->data[i][j].b = y->data[i][j] + 1.7721604*((float)cb->data[i][j]-128) + 0.0009902*((float)cr->data[i][j]-128);
    }
  return rgb;
}
