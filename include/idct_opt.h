#pragma once

#include <stdint.h>
#include <jpeg2ppm.h>


// Retourne l'IDCT de <mcu> sous forme de bloc 8x8 d'entiers 8 bits non signés
bloctu8_t *idct_opt(bloct16_t *mcu);
