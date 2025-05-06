#pragma once

#include "entete.h"
#include "jpeg2ppm.h"

bloct_t ***upsampler(bloct_t **cb, bloct_t **cr, uint64_t nbBlockCbH, uint64_t nbBlockCbV, uint64_t nbBlockCrH, uint64_t nbBlockCrV, idcomp_t* yfact, idcomp_t* cbfact, idcomp_t* crfact);
