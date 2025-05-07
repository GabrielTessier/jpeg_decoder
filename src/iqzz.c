#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "iqzz.h"
#include "jpeg2ppm.h"


// Parcours zig-zag en même temps que la quantification inverse
/*bloct_t *iqzz(blocl_t *mcuq, qtable_t *qtable) {
  bloct_t *res = (bloct_t*) malloc(sizeof(bloct_t));
  int i=0, j=0, k=1, dir=0;
  for (int ix=0; ix < 64; ix++) {
    if (dir == 0) {		// sens descendant
      if (i == 7)		// changement de sens
        j+=1, dir=1, k+=1;
      else if (i == k-1)	// changement de sens
        i+=1, dir=1, k+=1;
      else			// parcours normal
        i+=1, j-=1;
    } else if (dir == 1) {	// sens montant
      if (j == 7)		// changement de sens
        i-=1, dir=0, k+=1;
      else if (j == k-1)
        j+=1, dir=0, k+=1;	// changement de sens
      else			// parcours normal
        i-=1, j+=1;
    }
    res->data[i][j] = mcuq->data[ix] * qtable->data[ix];
  }
  return res;
  }*/

blocl16_t *iquant(blocl16_t *entree, qtable_t *qtable) {
  blocl16_t *res = (blocl16_t*) malloc(sizeof(blocl16_t));
  for (int i=0; i<64; i++) {
    res->data[i] = entree->data[i] * qtable->data[i];
  }
  return res;
}


bloct16_t *izz(blocl16_t *entree) {
  bloct16_t *res = (bloct16_t*) malloc(sizeof(bloct16_t));
  int i=0, j=0, k=1, dir=1;
  for (int ix=0; ix < 64; ix++) {
    res->data[j][i] = entree->data[ix];
    if (dir == 0) {        // sens descendant
      if (i == 7)          // changement de sens
        j+=1, dir=1, k+=1;
      else if (i == k-1)   // changement de sens
        i+=1, dir=1, k+=1;
      else                 // parcours normal
        i+=1, j-=1;
    } else if (dir == 1) { // sens montant
      if (j == 7)          // changement de sens
        i+=1, dir=0, k+=1;
      else if (j == k-1)
        j+=1, dir=0, k+=1; // changement de sens
      else                 // parcours normal
        i-=1, j+=1;
    }
   }
  return res;
}
