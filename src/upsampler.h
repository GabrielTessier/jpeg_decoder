#pragma once

#include "jpeg2ppm.h"

bloct_t ***upsampler(bloct_t **cb, bloct_t **cr, uint64_t nbBlockH, uint64_t nbBlockV, idcomp_t* yfact, idcomp_t* cbfact, idcomp_t* crfact);
