#include <math.h>
#include "idct.h"
#include "ycc2rgb.h"

// In:  mcudct_t*
// Out: ycbcr

float func_C(int khi) { return (khi == 0) ? 1 / sqrt(2) : 1; }

mcuycc_t idct(mcudct_t *freq) {
  mcuycc_t res;
  for (int x=0; x < 8; x++)
    for (int y=0; y < 8; y++) {
      float sum = 0; // double somme
      for (int lambda=0; lambda < 8; lambda++)
        for (int mu=0; mu < 8; mu++) {
          sum += func_C(lambda) * func_C(mu) *
            cos((2*x+1)*lambda*M_PI / 16) *
            cos((2*y+1)*mu*M_PI / 16) *
            freq[lambda][mu];
      res[x][y] = (uint8_t) 1/4 * sum + 128; // calcul de S(x,y) + offset
        }
    }
  return res;
}
