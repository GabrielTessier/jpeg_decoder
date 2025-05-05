#include <stdint.h>
#include <stdlib.h>
#include "ycc2rgb.h"

mcurgb_t *ycc2rgb(mcuycc_t y[64], mcuycc_t cb[64], mcuycc_t cr[64]) {
  mcurgb_t *rgb = (mcurgb_t*) malloc(sizeof(mcurgb_t));
  for (int i=0; i<8; i++) 
    for (int j=0; j<8; j++) {
      rgb->data[i][j].r = y->data[i][j] - 0.0009267*((float)cb->data[i][j]-128) + 1.4016868*((float)cr->data[i][j]-128);
      rgb->data[i][j].g = y->data[i][j] - 0.3436954*((float)cb->data[i][j]-128) - 0.7141690*((float)cr->data[i][j]-128);
      rgb->data[i][j].b = y->data[i][j] + 1.7721604*((float)cb->data[i][j]-128) + 0.0009902*((float)cr->data[i][j]-128);
    }
  return rgb;
}
