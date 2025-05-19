#pragma once

#include <stdint.h>
#include <jpeg2ppm.h>


// Remplit <stockage_coef> des coefficients
// C(lambda)*C(mu) * cos((2x+1)*lambda*PI/16) * sin((2x+1)*lambda*PI/16)
// de coordonnée (x, y, lambda, mu) pour le calcul de l'IDCT
void calc_coef(float stockage_coef[8][8][8][8]);

// Retourne l'IDCT de <bloc_freq> sous forme de bloc 8x8 d'entiers 8 bits non signés
// à l'aide des coefficient de <stockage_coefs> précalculés
bloctu8_t *idct(bloct16_t *bloc_freq, float stockage_coef[8][8][8][8]);
