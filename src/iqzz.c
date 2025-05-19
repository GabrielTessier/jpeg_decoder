#include <stdlib.h>
#include <stdint.h>

#include <iqzz.h>
#include <jpeg2ppm.h>
#include <img.h>


void iquant(blocl16_t *entree, uint8_t s_start, uint8_t s_end, qtable_t *qtable) {
   for (int i=s_start; i<=s_end; i++) {
      // Déquantification
      entree->data[i] = entree->data[i] * qtable->data[i];
   }
}

bloct16_t *izz(blocl16_t *entree) {
   bloct16_t *res = (bloct16_t*) malloc(sizeof(bloct16_t));
   int i=0, j=0; // indices du tableau de sortie
   int k=1;      // indice de la diagonale parcourue
   int dir=1;    // direction du parcours de la diagonale
   // 1: montant, 0: descendant
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
