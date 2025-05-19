#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include <idct_opt.h>
#include <idct.h>
#include "test_utils.h"

// Compare le résultat de l'IDCT rapide à celui de l'IDCT "classique"
// On teste la correspondance sur 10 000 mcu aléatoires.
// On accepte un écart entre les deux résultats de 1 maximum.

int main(int argc, char *argv[]) {
  (void) argc; // Pour éviter un warning à la compilation
  float stockage_coef[8][8][8][8];
  calc_coef(stockage_coef);
  bloct16_t *mcu = (bloct16_t *) malloc(sizeof(bloct16_t));
  int test_idct_opt = true;
  bloctu8_t *ref;
  bloctu8_t *res;
  int ncomp = 10000;
  int r = 0;
  while (r<ncomp && test_idct_opt) {
    for (int i=0; i<8; i++) {
      for (int j=0; j<8; j++) {
        mcu->data[i][j] = (uint8_t) rand();
      }
    }
    ref = idct(mcu, stockage_coef);
    res = idct_opt(mcu);
    int i=0;
    while (i<8 && test_idct_opt) {
      for (int j=0; j<8; j++) {
        // comparaison des résultats, écart de 1 maximum
        if (abs(ref->data[i][j] - res->data[i][j]) > 1) {
          test_idct_opt = false;
          break;
        }
      }
      i++;
    }
    free(ref);
    free(res);
    r++;
  }
  free(mcu);
  test_res(test_idct_opt, argv, "idct fast comparée à idct 'classique' (%d blocs 8x8 aléatoires)", ncomp);
  return 0;
}
