#include <stdint.h>

#include <ycc2rgb.h>
#include <jpeg2ppm.h>


// Retourne <val> si <val> est dans l'intervalle [0, 255], sinon retourne la
// borne de l'intervalle la plus proche de <val>.
static double clamp(double val);


static double clamp(double val) {
   if (val < 0) return 0;
   if (val > 255) return 255;
   return val;
}

void ycc2rgb_pixel(uint8_t y, uint8_t cb, uint8_t cr, rgb_t *rgb) {
   rgb->r = (uint8_t) clamp((double)y + 1.402*((double)cr-128));
   rgb->g = (uint8_t) clamp((double)y - 0.34414*((double)cb-128) - 0.71414*((double)cr-128));
   rgb->b = (uint8_t) clamp((double)y + 1.772*((double)cb-128));
}
