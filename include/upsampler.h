#pragma once

#include "entete.h"
#include "jpeg2ppm.h"

/* Entrées :
 * cb         : tableau 8x8 contenant les Cb
 * cr         : tableau 8x8 contenant les Cr
 * nbBlockCbH : nombre de blocs horizontaux de Cb
 * nbBlockCbV : nombre de blocs verticaux de Cb
 * nbBlockCrH : nombre de blocs horizontaux de Cr
 * nbBlockCrV : nombre de blocs verticaux de Cr
 * yfact      : structure contenant les infos de Y
 * cbfact     : structure contenant les infos de Cb
 * crfact     : structure contenant les infos de Cr
 */
//bloctu8_t ***upsampler(bloctu8_t **cb, bloctu8_t **cr, uint64_t nbBlockCbH, uint64_t nbBlockCbV, uint64_t nbBlockCrH, uint64_t nbBlockCrV, idcomp_t* yfact, idcomp_t* cbfact, idcomp_t* crfact);
bloctu8_t ***upsampler(img_t *img, bloctu8_t ***ycc);
