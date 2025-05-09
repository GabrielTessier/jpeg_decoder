#include <stdint.h>
#include <math.h>
#include "jpeg2ppm.h"
#include "entete.h"
#include "upsampler.h"

// Retourne un tableau 1x2 {Blocs(Cb), Blocs(Cr)} upsamplés
bloctu8_t ***upsampler(img_t *img, bloctu8_t ***ycc) {
  uint8_t nbcomp = img->comps->nb;
  bloctu8_t ***res = (bloctu8_t***) malloc(sizeof(bloctu8_t**)*nbcomp);
  // Nombre du bloc horizontalement et verticalement
  // (Plus petit que le vrai nombre car le prend pas en compte les MCU)
  int nbBlocH = ceil((float)img->width / 8);
  int nbBlocV = ceil((float)img->height / 8);
  // Calcul du hsampling et vsampling maximal
  uint8_t max_hsampling = 0;
  uint8_t max_vsampling = 0;
  for (int i=0; i<nbcomp; i++) {
    if (img->comps->comps[i]->hsampling > max_hsampling) max_hsampling = img->comps->comps[i]->hsampling;
    if (img->comps->comps[i]->vsampling > max_vsampling) max_vsampling = img->comps->comps[i]->vsampling;
  }
  // Nombre de MCU horizontalement et verticalement
  int nbmcuH = ceil((float)nbBlocH / max_hsampling);
  int nbmcuV = ceil((float)nbBlocV / max_vsampling);
  // Vrai nombre de bloc horizontalement et verticalement
  int reelnbBlocH = nbmcuH * max_hsampling;
  int reelnbBlocV = nbmcuV * max_vsampling;

  for (int c=0; c<nbcomp; c++) {
    res[c] = (bloctu8_t**) malloc(reelnbBlocH*reelnbBlocV * sizeof(bloctu8_t*));
    for (int j=0; j<reelnbBlocH*reelnbBlocV; j++) {
      res[c][j] = (bloctu8_t*) malloc(sizeof(bloctu8_t));
    }
    idcomp_t *comp = img->comps->comps[c];
    uint8_t facteurH = max_hsampling / comp->hsampling;
    uint8_t facteurV = max_vsampling / comp->vsampling;
    uint64_t nbBlocCompH = nbmcuH * comp->hsampling;
    uint64_t nbBlocCompV = nbmcuV * comp->vsampling;
  
    for (uint64_t i=0; i<nbBlocCompH; i++) {   // parcours des colonnes de blocs dans la composante
      for (uint64_t j=0; j<nbBlocCompV; j++) { // parcours des lignes de blocs dans la composante
	for (uint8_t k=0; k<8; k++) {         // parcours des colonnes d'un bloc
	  for (uint8_t l=0; l<8; l++) {       // parcours des lignes d'un bloc
	    uint64_t px = i*8+k;              // position x du pixel considéré dans la composante
	    uint64_t py = j*8+l;              // position y du pixel considéré dans la composante
	    uint64_t dpx = px*facteurH;     // position x du pixel considéré dans res
	    uint64_t dpy = py*facteurV;     // position y du pixel considéré dans res
	    for (uint8_t offX=0; offX<facteurH; offX++) {   // parcours des colonnes où imprimer le pixel considéré dans res
	      for (uint8_t offY=0; offY<facteurV; offY++) { // parcours des lignes où imprimer le pixel considéré dans res
		uint64_t blocId = ((dpy+offY)/8) * reelnbBlocH + ((dpx+offX)/8);
		res[c][blocId]->data[(dpx+offX)%8][(dpy+offY)%8] = ycc[c][j*nbBlocCompH+i]->data[k][l];
	      }
	    }
	  }
	}
      }
    }
  }

  return res;
}
