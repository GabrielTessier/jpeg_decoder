#include <stdint.h>
#include <stdlib.h>

#include <jpeg2ppm.h>
#include <idct_opt.h>

#define M_PI   3.14159265358979323846
#define SQRT_2 1.4142135623730950488
#define SQRT_8 2.8284271247461900976


// Opération 'butterfly' inverse de Loeffler (en place)
static void Loeffler_iX(float *i0, float *i1);
// Opération de rotation inverse de Loeffler (en place)
static void Loeffler_iC(float *i0, float *i1, float k, int n);
// Opération de dilatation inverse de Loeffler (en place)
static void Loeffler_iO(float *i0);
// Réordonne les composants de <coefs> pour inverser l'IDCT de Loeffler (en place)
static void reorder(float coefs[8]);
// Applique l'IDCT de Loeffler en 1D sur <coefs> (en place)
static void idct_opt_1D(float coefs[8]);
// Transpose <mat>
static void transpose(float mat[8][8]);



static const float fast_idct_coef[2][7] = {
   {0, 0.9807852804, 0, 0.8314696123, 0, 0, 0.3826834324},
   {0, 0.195090322, 0, 0.555570233, 0, 0, 0.9238795325}
};

static void Loeffler_iX(float *i0, float *i1) {
   float t0 = *i0, t1 = *i1;
   *i0 = (t0 + t1) / 2;
   *i1 = (t0 - t1) / 2;
}

static void Loeffler_iC(float *i0, float *i1, float k, int n) {
   float t0 = *i0, t1 = *i1;
   float tcos = fast_idct_coef[0][n], tsin = fast_idct_coef[1][n];
   *i0 = t0 / k * tcos - t1 / k * tsin;
   *i1 = t1 / k * tcos + t0 / k * tsin;
}

static void Loeffler_iO(float *i0) {
   *i0 = (*i0) / SQRT_2;
}

static void reorder(float coefs[8]) {
   float temp[8] = {coefs[0], coefs[4], coefs[2], coefs[6], coefs[7], coefs[3], coefs[5], coefs[1]};
   for (int i=0; i<8; i++) {
      coefs[i] = temp[i];
   }
}

static void idct_opt_1D(float coefs[8]) {
   reorder(coefs);
   // inversion étape 4
   Loeffler_iX(coefs+7, coefs+4);
   Loeffler_iO(coefs+5);
   Loeffler_iO(coefs+6);  
   // inversion étape 3
   Loeffler_iX(coefs+0, coefs+1);
   Loeffler_iC(coefs+2, coefs+3, SQRT_2, 6);
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
      coefs[i] *= SQRT_8;
   }
}

static void transpose(float mat[8][8]) {
   for (int i=0; i<8; i++) {
      for (int j=i+1; j<8; j++) {
	 float temp = mat[i][j];
	 mat[i][j] = mat[j][i];
	 mat[j][i] = temp;
      }
   }
}

bloctu8_t *idct_opt(bloct16_t *mcu) {
   float res[8][8];
   for (int i=0; i<8; i++) {
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
	 float x = res[j][i] + 128;
	 if (x < 0) x = 0;
	 if (x > 255) x = 255;
	 res2->data[i][j] = (uint8_t) x;
      }
   }
   return res2;
}
