#include <stdint.h>
#include <stdlib.h>

#include <idct_opt.h>
#include <idct.h>
#include "test_utils.h"

// Compare le résultat de l'IDCT rapide à celui de l'IDCT "normale"

int main(int argc, char *argv[]) {
  // pour éviter un warning à la compilation
  (void) argc;
  bloct16_t *mcu = (bloct16_t *) malloc(sizeof(bloct16_t));
  for (int i=0; i<8; i++) {
    for (int j=0; j<8; j++) {
      mcu->data[i][j] = (uint8_t) i + 8*j;
    }
  }
  float stockage_coef[8][8][8][8];
  calc_coef(stockage_coef);
  bloctu8_t *ref = idct(mcu, stockage_coef);
  bloctu8_t *res = idct_opt(mcu);
  free(mcu);
  int test_idct_opt = 1;
  for (int i=0; i<8 && test_idct_opt; i++) {
    for (int j=0; j<8; j++) {
      if (ref->data[i][j] != res->data[i][j]) {
        test_idct_opt = 0;
        break;
      }
    }
  }
  free(ref);
  free(res);
  test_res(test_idct_opt, argv, "idct fast comparée à idct 'classique'");
  return 0;
}
