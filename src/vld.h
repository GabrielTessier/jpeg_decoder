#pragma once

#include <stdio.h>
#include <stdint.h>


struct huffman_tree_s {
  struct huffman_tree_s *gauche, *droit;
  uint8_t symb;
};
typedef struct huffman_tree_s huffman_tree_t;


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
