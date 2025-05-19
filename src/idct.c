#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include <jpeg2ppm.h>
#include <idct.h>

#define M_PI 3.14159265358979323846


// Retourne C(lambda)*C(mu)
static float f_C(int lambda, int mu);

// Retourne un tableau 8x8 contenant les coefficients cos((2*x+1)*lambda*PI / 16) pour lambda, x dans [|0, 7|]
static void calc_cos(float stockage_cos[8][8]);



static float f_C(int lambda, int mu) {
   if (lambda == 0) {
      return (mu == 0) ? 0.5 : 1/sqrt(2);
   }
   return (mu == 0) ?  1/sqrt(2) : 1;
}

static void calc_cos(float stockage_cos[8][8]) {
   for (int x=0; x < 8; x++) {
      for (int lambda=0; lambda < 8; lambda++) {
	 stockage_cos[x][lambda] = cos((2*x+1)*lambda*M_PI / 16);
      }
   }
}

void calc_coef(float stockage_coef[8][8][8][8]) {
   float stockage_cos[8][8];
   calc_cos(stockage_cos);
   for (int x=0; x < 8; x++) {
      for (int y=0; y < 8; y++) {
	 for (int lambda=0; lambda < 8; lambda++) {
	    for (int mu=0; mu < 8; mu++) {
	       stockage_coef[x][y][lambda][mu] = f_C(lambda,mu) *
		  stockage_cos[x][lambda] * 
		  stockage_cos[y][mu];
	    }
	 }
      }
   }
}

bloctu8_t *idct(bloct16_t *bloc_freq, float stockage_coef[8][8][8][8]) {
   bloctu8_t *res = (bloctu8_t*) malloc(sizeof(bloctu8_t));
   for (int x=0; x < 8; x++) {
      for (int y=0; y < 8; y++) {
	 float sum = 0; // double somme
	 for (int lambda=0; lambda < 8; lambda++) {
	    for (int mu=0; mu < 8; mu++) {
	       float val = stockage_coef[x][y][lambda][mu];
	       val *= bloc_freq->data[lambda][mu];
	       sum += val;
	    }
	 }
	 sum *= 0.25;
	 sum += 128;
	 // clamping de sum
	 if (sum < 0) sum = 0;
	 if (sum > 255) sum = 255;
	 res->data[x][y] = (uint8_t) sum;
      }
   }
   return res;
}
