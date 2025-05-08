#pragma once

#include <stdio.h>
#include <stdint.h>
#include "jpeg2ppm.h"

struct huffman_tree_s {
  struct huffman_tree_s *gauche, *droit;
  uint8_t symb;
};
typedef struct huffman_tree_s huffman_tree_t;


blocl16_t *decode_bloc_acdc(FILE *fichier, huffman_tree_t *hdc, huffman_tree_t *hac, int16_t *dc_prec, uint64_t *debut, uint8_t *off);

/* Décode tous les DC
 * ht : arbre de huffman
 * file : fichier
 * pos : position du début du code dans le fichier
 * size : nombre de dc à décoder */
int16_t *decodeDC(huffman_tree_t* ht, FILE* file, uint64_t pos, uint8_t *off, uint64_t size);


/* Décode un macrobloc
 * ht : arbre de huffman
 * file : fichier
 * pos : position du début du code dans le fichier */
int16_t *decodeAC(huffman_tree_t* ht, FILE* file, uint64_t pos, uint8_t *off);
