#include <stdint.h>
#include <stdlib.h>
#include "ycc2rgb.h"
#include "jpeg2ppm.h"

bloc_rgb_t *ycc2rgb_bloc(bloctu8_t y[64], bloctu8_t cb[64], bloctu8_t cr[64]) {
  bloc_rgb_t *rgb = (bloc_rgb_t*) malloc(sizeof(bloc_rgb_t));
  for (int i=0; i<8; i++) 
    for (int j=0; j<8; j++) {
      rgb->data[i][j].r = y->data[i][j] - 0.0009267*((float)cb->data[i][j]-128) + 1.4016868*((float)cr->data[i][j]-128);
      rgb->data[i][j].g = y->data[i][j] - 0.3436954*((float)cb->data[i][j]-128) - 0.7141690*((float)cr->data[i][j]-128);
      rgb->data[i][j].b = y->data[i][j] + 1.7721604*((float)cb->data[i][j]-128) + 0.0009902*((float)cr->data[i][j]-128);
    }
  return rgb;
}

double clamp(double val, double min, double max) {
  if (val < min) return min;
  if (val > max) return max;
  return val;
}

rgb_t *ycc2rgb_pixel(uint8_t y, uint8_t cb, uint8_t cr) {
  rgb_t *rgb = (rgb_t*) malloc(sizeof(rgb_t));
  //rgb->r = clamp((float)y - 0.0009267*((float)cb-128) + 1.4016868*((float)cr-128), 0, 255);
  //rgb->g = clamp((float)y - 0.3436954*((float)cb-128) - 0.7141690*((float)cr-128), 0, 255);
  //rgb->b = clamp((float)y + 1.7721604*((float)cb-128) + 0.0009902*((float)cr-128), 0, 255);
  rgb->r = (uint8_t) clamp((double)y + 1.402*((double)cr-128), 0, 255);
  rgb->g = (uint8_t) clamp((double)y - 0.34414*((double)cb-128) - 0.71414*((double)cr-128), 0, 255);
  rgb->b = (uint8_t) clamp((double)y + 1.772*((double)cb-128), 0, 255);
  return rgb;
}
