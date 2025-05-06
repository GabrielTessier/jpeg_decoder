#pragma once
#include <stdio.h>
#include <stdint.h>

struct huffman_tree_dc_s {
  struct huffman_tree_dc_s *gauche, *droit;
  uint8_t magnitude;
};
typedef struct huffman_tree_dc_s huffman_tree_dc_t;

/* RLE_EOB : End Of Block
 * RLE_VIDE : 16 coefficients nuls
 * RLE_ALPHAGAMMA : 0x alpha gamma -> alpha coefficients nuls, puis coefficient
 *                                    non nul de magnitude gamma
 */
enum symbRLE {RLE_EOB, RLE_VIDE, RLE_ALPHAGAMMA};

struct huffman_tree_ac_s {
  struct huffman_tree_ac_s *gauche, *droit;
  enum symbRLE symb;
  uint8_t alpha;
  uint8_t gamma;
};
typedef struct huffman_tree_ac_s huffman_tree_ac_t;

/* Décode tout les DC
 * ht : arbre de huffman
 * file : fichier
 * pos : position du début du code dans le fichier
 * size : nombre de dc à décoder */
int8_t *decodeDC(huffman_tree_dc_t* ht, FILE* file, uint64_t pos, uint64_t size);

/* Décode un macrobloc
 * ht : arbre de huffman
 * file : fichier
 * pos : position du début du code dans le fichier */
int8_t *decodeAC(huffman_tree_ac_t* ht, FILE* file, uint64_t pos);
