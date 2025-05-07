#pragma once

#include "entete.h"
#include "jpeg2ppm.h"

bloctu8_t ***upsampler(bloctu8_t **cb, bloctu8_t **cr, uint64_t nbBlockCbH, uint64_t nbBlockCbV, uint64_t nbBlockCrH, uint64_t nbBlockCrV, idcomp_t* yfact, idcomp_t* cbfact, idcomp_t* crfact);
