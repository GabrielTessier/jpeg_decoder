#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include "jpeg2ppm.h"
#include "idct_opt.h"

#define M_PI 3.14159265358979323846
#define R_2  1.4142135623730951
#define R_8  2.82842712475

void Loeffler_iX(float *i0, float *i1) {
  float t0 = *i0, t1 = *i1;
  *i0 = (t0 + t1) / 2;
  *i1 = (t0 - t1) / 2;
}

void Loeffler_iC(float *i0, float *i1, float k, float n) {
  float t0 = *i0, t1 = *i1;
  *i0 = t0 / k * cos(n*M_PI/16) - t1 / k * sin(n*M_PI/16);
  *i1 = t0 / k * sin(n*M_PI/16) + t1 / k * cos(n*M_PI/16);
}

void Loeffler_iO(float *i0) {
  *i0 = (*i0) / R_2;
}

void idct_opt_1D(float coefs[8]) {
  // inversion étape 4
  Loeffler_iX(coefs+7, coefs+4);
  Loeffler_iO(coefs+5);
  Loeffler_iO(coefs+6);  
  // inversion étape 3
  Loeffler_iX(coefs+0, coefs+1);
  Loeffler_iC(coefs+2, coefs+3, R_2, 6);
  Loeffler_iX(coefs+4, coefs+6);
  Loeffler_iX(coefs+7, coefs+5);
  // inversion étape 2
  Loeffler_iX(coefs+0, coefs+3);
  Loeffler_iX(coefs+1, coefs+2);
  Loeffler_iC(coefs+4, coefs+7, 1, 3);
  Loeffler_iC(coefs+5, coefs+6, 1, 1);
  //inversion étape 1
  Loeffler_iX(coefs+0, coefs+7);
  Loeffler_iX(coefs+1, coefs+6);
  Loeffler_iX(coefs+2, coefs+5);
  Loeffler_iX(coefs+3, coefs+4);
  // normalisation
  for (int i=0; i<8; i++) {
    coefs[i] /= R_8;
  }
 }

void transpose(float **mat) {
  for (int i=0; i<8; i++) {
    for (int j=i+1; j<8; j++) {
      float temp = mat[i][j];
      mat[i][j] = mat[j][i];
      mat[j][i] = temp;
    }
  }
}

bloctu8_t *idct_opt(bloct16_t *mcu) {
  float **res = (float**) malloc(sizeof(float*)*8);
  for (int i=0; i<8; i++) {
    res[i] = (float *) malloc(sizeof(float)*8);
    for (int j=0; j<8; j++) {
      res[i][j] = (float) mcu->data[i][j];
    }
  }
  for (int i=0; i<8; i++) {
    idct_opt_1D(res[i]);
  }
  transpose(res);
  for (int i=0; i<8; i++) {
    idct_opt_1D(res[i]);
  }
  bloctu8_t *res2 = (bloctu8_t *) malloc(sizeof(bloctu8_t));
  for (int i=0; i<8; i++) {
    for (int j=0; j<8; j++) {
      res2->data[i][j] = (uint8_t) res[j][i];
    }
  }
  return res2;
}


float **calc_cos();
float ****calc_coef();
