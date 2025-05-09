#pragma once

#include <stdio.h>
#include <stdint.h>
#include "jpeg2ppm.h"

struct huffman_tree_s {
  struct huffman_tree_s *gauche, *droit;
  uint8_t symb;
};
typedef struct huffman_tree_s huffman_tree_t;

enum acdc_e { AC, DC };

int16_t *decode_list_coef(huffman_tree_t* ht, FILE* file, uint8_t *off, enum acdc_e type);

blocl16_t *decode_bloc_acdc(FILE *fichier, huffman_tree_t *hdc, huffman_tree_t *hac, int16_t *dc_prec, uint8_t *off);
