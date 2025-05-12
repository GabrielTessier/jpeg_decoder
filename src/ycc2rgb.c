#include <stdint.h>
#include <stdlib.h>

#include <ycc2rgb.h>
#include <jpeg2ppm.h>


double clamp(double val, double min, double max) {
   if (val < min) return min;
   if (val > max) return max;
   return val;
}

rgb_t *ycc2rgb_pixel(uint8_t y, uint8_t cb, uint8_t cr) {
   rgb_t *rgb = (rgb_t*) malloc(sizeof(rgb_t));
   rgb->r = (uint8_t) clamp((double)y + 1.402*((double)cr-128), 0, 255);
   rgb->g = (uint8_t) clamp((double)y - 0.34414*((double)cb-128) - 0.71414*((double)cr-128), 0, 255);
   rgb->b = (uint8_t) clamp((double)y + 1.772*((double)cb-128), 0, 255);
   return rgb;
}
