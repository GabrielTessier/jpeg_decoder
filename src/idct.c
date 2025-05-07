#include <math.h>
#include <stdlib.h>
#include "jpeg2ppm.h"
#include "idct.h"

#define M_PI       3.14159265358979323846

// In:  mcudct_t*
// Out: ycbcr

float func_C(int khi) { return (khi == 0) ? 1 / sqrt(2) : 1; }

float f_C(int lambda, int mu) {
  if (lambda == 0) {
    return (mu == 0) ? 0.5 : 1/sqrt(2);
  }
  return (mu == 0) ?  1/sqrt(2) : 1;
}

float **calc_cos() {
  float **stockage_cos = malloc(sizeof(float*)*8);
  for (int x=0; x < 8; x++) {
    stockage_cos[x] = malloc(sizeof(float)*8);
    for (int lambda=0; lambda < 8; lambda++) {
      stockage_cos[x][lambda] = cos((2*x+1)*lambda*M_PI / 16);
    }
  }
  return stockage_cos;
}

float ****calc_coef() {
  float ****stockage_coef = malloc(sizeof(float***)*8);
  float **stockage_cos = calc_cos();
  for (int x=0; x < 8; x++) {
    stockage_coef[x] = malloc(sizeof(float**)*8);
    for (int y=0; y < 8; y++) {
      stockage_coef[x][y] = malloc(sizeof(float*)*8);
      for (int lambda=0; lambda < 8; lambda++) {
        stockage_coef[x][y][lambda] = malloc(sizeof(float)*8);
        for (int mu=0; mu < 8; mu++) {
          stockage_coef[x][y][lambda][mu] = f_C(lambda,mu) *
                                            stockage_cos[x][lambda] * 
                                            stockage_cos[y][mu];
        }
      }
    }
  }
  return stockage_coef;
}

bloctu8_t *idct(bloct16_t *freq) {
  float ****stockage_coef = calc_coef();
  bloctu8_t *res = (bloctu8_t*) malloc(sizeof(bloctu8_t));
  for (int x=0; x < 8; x++)
    for (int y=0; y < 8; y++) {
      float sum = 0; // double somme
      for (int lambda=0; lambda < 8; lambda++)
        for (int mu=0; mu < 8; mu++) {
          sum += stockage_coef[x][y][lambda][mu] * freq->data[lambda][mu];
        }
      //res->data[x][y] = (uint8_t) (0.25 * sum + 128); // calcul de S(x,y) + offset
      sum *= 0.25;
      sum += 128;
      if (sum < 0) sum = 0;
      if (sum > 255) sum = 255;
      res->data[x][y] = sum;
    }
  return res;
}
