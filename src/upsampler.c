#include <stdint.h>
#include <math.h>
#include "jpeg2ppm.h"
#include "entete.h"
#include "upsampler.h"

// Retourne un tableau 1x2 {Blocs(Cb), Blocs(Cr)} upsamplés
bloctu8_t ***upsampler(bloctu8_t **cb, bloctu8_t **cr, img_t *img) {
  bloctu8_t ***res = (bloctu8_t***) malloc(sizeof(bloctu8_t**)*2);
  idcomp_t *ycomp = img->comps->comps[0];
  idcomp_t *cbcomp = img->comps->comps[1];
  idcomp_t *crcomp = img->comps->comps[2];
  
  int nbBlocYH = ceil((float)img->width / 8);
  int nbBlocYV = ceil((float)img->height / 8);
  int nbmcuH = ceil((float)nbBlocYH / ycomp->hsampling);
  int nbmcuV = ceil((float)nbBlocYV / ycomp->vsampling);
  int reelnbBlocYH = nbmcuH * ycomp->hsampling;
  int reelnbBlocYV = nbmcuV * ycomp->vsampling;
  
  res[0] = (bloctu8_t**) calloc(reelnbBlocYH*reelnbBlocYV, sizeof(bloctu8_t*));
  res[1] = (bloctu8_t**) calloc(reelnbBlocYH*reelnbBlocYV, sizeof(bloctu8_t*));

  uint8_t facteurCbH = ycomp->hsampling / cbcomp->hsampling;
  uint8_t facteurCbV = ycomp->vsampling / cbcomp->vsampling;
  uint8_t facteurCrH = ycomp->hsampling / crcomp->hsampling;
  uint8_t facteurCrV = ycomp->vsampling / crcomp->vsampling;
  printf("%d, %d; %d, %d\n", facteurCbH, facteurCbV, facteurCrH, facteurCrV);
  uint64_t nbBlocCbH = nbmcuH * cbcomp->hsampling;
  uint64_t nbBlocCbV = nbmcuV * cbcomp->vsampling;
  uint64_t nbBlocCrH = nbmcuH * crcomp->hsampling;
  uint64_t nbBlocCrV = nbmcuV * crcomp->vsampling;
  
  for (uint64_t i=0; i<nbBlocCbH; i++) {   // parcours des colonnes de blocs dans cb
    for (uint64_t j=0; j<nbBlocCbV; j++) { // parcours des lignes de blocs dans cb
      for (uint8_t k=0; k<8; k++) {         // parcours des colonnes d'un bloc
        for (uint8_t l=0; l<8; l++) {       // parcours des lignes d'un bloc
          uint64_t px = i*8+k;              // position x du pixel considéré dans cb
          uint64_t py = j*8+l;              // position y du pixel considéré dans cb
          uint64_t dpx = px*facteurCbH;     // position x du pixel considéré dans res
          uint64_t dpy = py*facteurCbV;     // position y du pixel considéré dans res
          for (uint8_t offX=0; offX<facteurCbH; offX++) {   // parcours des colonnes où imprimer le pixel considéré dans res
            for (uint8_t offY=0; offY<facteurCbV; offY++) { // parcours des lignes où imprimer le pixel considéré dans res
	      uint64_t blocId = ((dpy+offY)/8) * reelnbBlocYH + ((dpx+offX)/8);
	      if (res[0][blocId] == NULL) res[0][blocId] = (bloctu8_t*) malloc(sizeof(bloctu8_t));
              res[0][blocId]->data[(dpx+offX)%8][(dpy+offY)%8] = cb[j*nbBlocCbH+i]->data[k][l];
	    }
	  }
        }
      }
    }
  }

  for (uint64_t i=0; i<nbBlocCrH; i++) {   // parcours des colonnes de blocs dans cb
    for (uint64_t j=0; j<nbBlocCrV; j++) { // parcours des lignes de blocs dans cb
      for (uint8_t k=0; k<8; k++) {         // parcours des colonnes d'un bloc
        for (uint8_t l=0; l<8; l++) {       // parcours des lignes d'un bloc
          uint64_t px = i*8+k;              // position x du pixel considéré dans cb
          uint64_t py = j*8+l;              // position y du pixel considéré dans cb
          uint64_t dpx = px*facteurCrH;     // position x du pixel considéré dans res
          uint64_t dpy = py*facteurCrV;     // position y du pixel considéré dans res
          for (uint8_t offX=0; offX<facteurCrH; offX++) {   // parcours des colonnes où imprimer le pixel considéré dans res
            for (uint8_t offY=0; offY<facteurCrV; offY++) { // parcours des lignes où imprimer le pixel considéré dans res
	      uint64_t blocId = ((dpy+offY)/8)*(nbBlocCrH*facteurCrH) + ((dpx+offX)/8);
	      if (res[1][blocId] == NULL) res[1][blocId] = (bloctu8_t*) malloc(sizeof(bloctu8_t));
              res[1][blocId]->data[(dpx+offX)%8][(dpy+offY)%8] = cr[j*nbBlocCrH+i]->data[k][l];
	    }
	  }
        }
      }
    }
  }

  return res;
}
