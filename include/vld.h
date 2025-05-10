#pragma once

#include <stdio.h>
#include <stdint.h>

#include <jpeg2ppm.h>

// Structure représentant un arbre de Huffman
struct huffman_tree_s {
  struct huffman_tree_s *gauche, *droit;
  uint8_t symb;
};
typedef struct huffman_tree_s huffman_tree_t;

// Enumération pour indiquer le type de traitement voulu
enum acdc_e { AC, DC };

// Retourne le bloc (AC et DC) commençant à <fichier>+<off>,
// décodé à l'aide des tables de Huffman <hdc> (DC) et <hac> (AC).
// On retrouve la composante constante de ce bloc fréquentiel
// à l'aide de <dc_prec>.
blocl16_t *decode_bloc_acdc(FILE *fichier, huffman_tree_t *hdc, huffman_tree_t *hac, int16_t *dc_prec, uint8_t *off);
