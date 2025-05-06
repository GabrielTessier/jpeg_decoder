#include <stdint.h>
#include "jpeg2ppm.h"
#include "upsampler.h"

bloct_t ***upsampler(bloct_t **cb, bloct_t **cr, uint64_t nbBlockH, uint64_t nbBlockV, idcomp_t* yfact, idcomp_t* cbfact, idcomp_t* crfact) {
  bloct_t ***res = (bloct_t***) malloc(sizeof(bloct_t**)*2);
  uint8_t nbBlockInMCU = yfact->hsampling * yfact->vsampling;
  uint8_t nbBlockInMCUcb = cbfact->hsampling * cbfact->vsampling;
  uint8_t nbBlockInMCUcr = crfact->hsampling * crfact->vsampling;
  uint8_t facteurcb = nbBlockInMCU/nbBlockInMCUcb;
  uint8_t facteurcr = nbBlockInMCU/nbBlockInMCUcr;
  res[0] = (bloct_t**) malloc(sizeof(bloct_t*)*facteurcb);
  res[1] = (bloct_t**) malloc(sizeof(bloct_t*)*facteurcr);
  // CB
  uint8_t facteurH = yfact->hsampling / cbfact->hsampling;
  uint8_t facteurV = yfact->vsampling / cbfact->vsampling;
  for (uint64_t i=0; i<nbBlockH; i++) {
    for (uint64_t j=0; j<nbBlockV; j++) {
      for (uint8_t k=0; k<8; k++) {
	for (uint8_t l=0; l<8; l++) {
	  uint64_t px = i*8+k;
	  uint64_t py = j*8+l;
	  uint64_t dpx = px*facteurH;
	  uint64_t dpy = px*facteurV;
	  for (uint8_t offX=0; offX<facteurH; offX++)
	    for (uint8_t offY=0; offY<facteurV; offY++)
	      res[0][((dpy+offY)/8)*nbBlockH*facteurH+((dpx+offX)/8)]->data[(dpx+offX)%8][(dpy+offY)%8] = cb[j*nbBlockH+i]->data[k][l];
	}
      }
    }
  }
}
