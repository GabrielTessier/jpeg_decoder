#include <stdlib.h>

#include <entete.h>
#include <jpeg2ppm.h>

// Retourne un tableau 1xnb_composante upsamplé
bloctu8_t ***upsampler(img_t *img, bloctu8_t ***ycc) {
  uint8_t nbcomp = img->comps->nb;
  bloctu8_t ***res = (bloctu8_t ***) malloc(sizeof(bloctu8_t**)*nbcomp);
  uint64_t nb_blocH = img->nbmcuH * img->max_hsampling;
  uint64_t nb_blocV = img->nbmcuV * img->max_vsampling;
  for (uint8_t i=0; i<nbcomp; i++) {
    res[i] = (bloctu8_t**) malloc(nb_blocH*nb_blocV * sizeof(bloctu8_t*));
    for (uint64_t j=0; j<nb_blocH*nb_blocV; j++) {
      res[i][j] = (bloctu8_t*) malloc(sizeof(bloctu8_t));
    }
  }
  //bloctu8_t ***res = (bloctu8_t***) malloc(sizeof(bloctu8_t**)*nbcomp);

  for (uint16_t c=0; c<nbcomp; c++) {
    idcomp_t *comp = img->comps->comps[c];
    uint8_t facteurH = img->max_hsampling / comp->hsampling;
    uint8_t facteurV = img->max_vsampling / comp->vsampling;
    uint64_t nb_bloc_compH = img->nbmcuH * comp->hsampling;
    uint64_t nb_bloc_compV = img->nbmcuV * comp->vsampling;

    if (facteurH == 1 && facteurV == 1) {
      for (uint64_t i=0; i<nb_blocH*nb_blocV; i++) {
        for (uint8_t k=0; k<8; k++) {
          for (uint8_t l=0; l<8; l++) {
            res[c][i]->data[k][l] = ycc[c][i]->data[k][l];
          }
        }
      }
    }
  
    for (uint64_t i=0; i<nb_bloc_compH; i++) {   // parcours des colonnes de blocs dans la composante
      for (uint64_t j=0; j<nb_bloc_compV; j++) { // parcours des lignes de blocs dans la composante
	for (uint8_t k=0; k<8; k++) {         // parcours des colonnes d'un bloc
	  for (uint8_t l=0; l<8; l++) {       // parcours des lignes d'un bloc
	    uint64_t px = i*8+k;              // position x du pixel considéré dans la composante
	    uint64_t py = j*8+l;              // position y du pixel considéré dans la composante
	    uint64_t dpx = px*facteurH;     // position x du pixel considéré dans res
	    uint64_t dpy = py*facteurV;     // position y du pixel considéré dans res
	    for (uint8_t offX=0; offX<facteurH; offX++) {   // parcours des colonnes où imprimer le pixel considéré dans res
	      for (uint8_t offY=0; offY<facteurV; offY++) { // parcours des lignes où imprimer le pixel considéré dans res
		uint64_t blocId = ((dpy+offY)/8) * nb_blocH + ((dpx+offX)/8);
		res[c][blocId]->data[(dpx+offX)%8][(dpy+offY)%8] = ycc[c][j*nb_bloc_compH+i]->data[k][l];
	      }
	    }
	  }
	}
      }
    }
  }

  return res;
}
