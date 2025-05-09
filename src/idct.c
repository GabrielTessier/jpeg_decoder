#include <math.h>
#include <stdlib.h>
#include "jpeg2ppm.h"
#include "idct.h"

#define M_PI 3.14159265358979323846

// Retourne C(lambda)*C(mu)
static float f_C(int lambda, int mu);
// Retourne un tableau 8x8 contenant les coefficients cos((2*x+1)*lambda*PI / 16) pour lambda, x dans [|0, 7|]
static float **calc_cos;

static float f_C(int lambda, int mu) {
  if (lambda == 0) {
    return (mu == 0) ? 0.5 : 1/sqrt(2);
  }
  return (mu == 0) ?  1/sqrt(2) : 1;
}

static float **calc_cos() {
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
  for (int x=0; x < 8; x++) {
    free(stockage_cos[x]);
  }
  free(stockage_cos);
  return stockage_coef;
}

bloctu8_t *idct(bloct16_t *freq, float ****stockage_coef) {
  bloctu8_t *res = (bloctu8_t*) malloc(sizeof(bloctu8_t));
  for (int x=0; x < 8; x++) {
    for (int y=0; y < 8; y++) {
      float sum = 0; // double somme
      for (int lambda=0; lambda < 8; lambda++) {
        for (int mu=0; mu < 8; mu++) {
          float val = stockage_coef[x][y][lambda][mu];
	  val *= freq->data[lambda][mu];
	  sum += val;
        }
      }
      //res->data[x][y] = (uint8_t) (0.25 * sum + 128); // calcul de S(x,y) + offset
      sum *= 0.25;
      sum += 128;
      if (sum < 0) sum = 0;
      if (sum > 255) sum = 255;
      res->data[x][y] = sum;
    }
  }
  return res;
}
