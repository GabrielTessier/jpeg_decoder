#include <stdint.h>
#include "jpeg2ppm.h"
#include "entete.h"
#include "upsampler.h"

// Retourne un tableau 1x2 {Blocs(Cb), Blocs(Cr)} upsamplés
bloctu8_t ***upsampler(bloctu8_t **cb, bloctu8_t **cr, uint64_t nbBlockCbH, uint64_t nbBlockCbV, uint64_t nbBlockCrH, uint64_t nbBlockCrV, idcomp_t* yfact, idcomp_t* cbfact, idcomp_t* crfact) {
  bloctu8_t ***res = (bloctu8_t***) malloc(sizeof(bloctu8_t**)*2);
  uint8_t nbBlockInMCU   = yfact->hsampling * yfact->vsampling;
  uint8_t nbBlockInMCUcb = cbfact->hsampling * cbfact->vsampling;
  uint8_t nbBlockInMCUcr = crfact->hsampling * crfact->vsampling;
  uint8_t facteurcb = nbBlockInMCU/nbBlockInMCUcb;
  uint8_t facteurcr = nbBlockInMCU/nbBlockInMCUcr;
  res[0] = (bloctu8_t**) malloc(sizeof(bloctu8_t*)*facteurcb);
  res[1] = (bloctu8_t**) malloc(sizeof(bloctu8_t*)*facteurcr);
  
  uint8_t facteurCbH = yfact->hsampling / cbfact->hsampling;
  uint8_t facteurCbV = yfact->vsampling / cbfact->vsampling;
  uint8_t facteurCrH = yfact->hsampling / cbfact->hsampling;
  uint8_t facteurCrV = yfact->vsampling / cbfact->vsampling;
  
  for (uint64_t i=0; i<nbBlockCbH; i++) {   // parcours des colonnes de blocs dans cb
    for (uint64_t j=0; j<nbBlockCbV; j++) { // parcours des lignes de blocs dans cb
      for (uint8_t k=0; k<8; k++) {         // parcours des colonnes d'un bloc
        for (uint8_t l=0; l<8; l++) {       // parcours des lignes d'un bloc
          uint64_t px = i*8+k;              // position x du pixel considéré dans cb
          uint64_t py = j*8+l;              // position y du pixel considéré dans cb
          uint64_t dpx = px*facteurCbH;     // position x du pixel considéré dans res
          uint64_t dpy = py*facteurCbV;     // position y du pixel considéré dans res
          for (uint8_t offX=0; offX<facteurCbH; offX++)   // parcours des colonnes où imprimer le pixel considéré dans res
            for (uint8_t offY=0; offY<facteurCbV; offY++) // parcours des lignes où imprimer le pixel considéré dans res
              res[0][((dpy+offY)/8)*(nbBlockCbH*facteurCbH) + ((dpx+offX)/8)]->data[(dpx+offX)%8][(dpy+offY)%8] = cb[j*nbBlockCbH+i]->data[k][l];
        }
      }
    }
  }

  for (uint64_t i=0; i<nbBlockCrH; i++) {
    for (uint64_t j=0; j<nbBlockCrV; j++) {
      for (uint8_t k=0; k<8; k++) {
        for (uint8_t l=0; l<8; l++) {
          uint64_t px = i*8+k;
          uint64_t py = j*8+l;
          uint64_t dpx = px*facteurCrH;
          uint64_t dpy = py*facteurCrV;
          for (uint8_t offX=0; offX<facteurCrH; offX++)
            for (uint8_t offY=0; offY<facteurCrV; offY++)
              res[1][((dpy+offY)/8)*(nbBlockCrH*facteurCrH) + ((dpx+offX)/8)]->data[(dpx+offX)%8][(dpy+offY)%8] = cr[j*nbBlockCrH+i]->data[k][l];
        }
      }
    }
  }

  return res;
}
