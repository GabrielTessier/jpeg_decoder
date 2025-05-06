#include <stdint.h>
#include "jpeg2ppm.h"
#include "entete.h"
#include "upsampler.h"

bloct_t ***upsampler(bloct_t **cb, bloct_t **cr, uint64_t nbBlockCbH, uint64_t nbBlockCbV, uint64_t nbBlockCrH, uint64_t nbBlockCrV, idcomp_t* yfact, idcomp_t* cbfact, idcomp_t* crfact) {
  bloct_t ***res = (bloct_t***) malloc(sizeof(bloct_t**)*2);
  uint8_t nbBlockInMCU = yfact->hsampling * yfact->vsampling;
  uint8_t nbBlockInMCUcb = cbfact->hsampling * cbfact->vsampling;
  uint8_t nbBlockInMCUcr = crfact->hsampling * crfact->vsampling;
  uint8_t facteurcb = nbBlockInMCU/nbBlockInMCUcb;
  uint8_t facteurcr = nbBlockInMCU/nbBlockInMCUcr;
  res[0] = (bloct_t**) malloc(sizeof(bloct_t*)*facteurcb);
  res[1] = (bloct_t**) malloc(sizeof(bloct_t*)*facteurcr);
  
  uint8_t facteurCbH = yfact->hsampling / cbfact->hsampling;
  uint8_t facteurCbV = yfact->vsampling / cbfact->vsampling;
  uint8_t facteurCrH = yfact->hsampling / cbfact->hsampling;
  uint8_t facteurCrV = yfact->vsampling / cbfact->vsampling;
  
  for (uint64_t i=0; i<nbBlockCbH; i++) {
    for (uint64_t j=0; j<nbBlockCbV; j++) {
      for (uint8_t k=0; k<8; k++) {
	for (uint8_t l=0; l<8; l++) {
	  uint64_t px = i*8+k;
	  uint64_t py = j*8+l;
	  uint64_t dpx = px*facteurCbH;
	  uint64_t dpy = py*facteurCbV;
	  for (uint8_t offX=0; offX<facteurCbH; offX++)
	    for (uint8_t offY=0; offY<facteurCbV; offY++)
	      res[0][((dpy+offY)/8)*nbBlockCbH*facteurCbH+((dpx+offX)/8)]->data[(dpx+offX)%8][(dpy+offY)%8] = cb[j*nbBlockCbH+i]->data[k][l];
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
	      res[1][((dpy+offY)/8)*nbBlockCrH*facteurCrH+((dpx+offX)/8)]->data[(dpx+offX)%8][(dpy+offY)%8] = cr[j*nbBlockCrH+i]->data[k][l];
	}
      }
    }
  }

  return res;
}
