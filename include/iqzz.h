#pragma once

#include <stdint.h>
#include <jpeg2ppm.h>


// Structure contenant une table de quantification dans un tableau 1x64 
struct qtable_s {
  uint16_t data[64];
};
typedef struct qtable_s qtable_t;

// Déquantifie le bloc <entree> de <s_start> à <s_end> à l'aide de la table de quantification <qtable>
void iquant(blocl16_t *entree, uint8_t s_start, uint8_t s_end, qtable_t *qtable);

// Retourne le bloc <entree> (un tableau 1x64) après
// avoir inversé l'opération de zig-zag, donc sous
// forme d'un tableau 8x8
bloct16_t *izz(blocl16_t *entree);
