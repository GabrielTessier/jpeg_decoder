#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <jpeg2ppm.h>
#include <erreur.h>

// Structure représentant un arbre de Huffman
struct huffman_tree_s {
  // fils[0] représente le fils gauche et fils[1] le fils droit
  struct huffman_tree_s *fils[2];
  uint8_t symb;
};
typedef struct huffman_tree_s huffman_tree_t;

// Enumération pour indiquer le type de traitement voulu
enum acdc_e { AC, DC };

// Retourne le bloc (AC et DC) commençant à <fichier>+<off>,
// décodé à l'aide des tables de Huffman <hdc> (DC) et <hac> (AC).
// On retrouve la composante constante de ce bloc fréquentiel
// à l'aide de <dc_prec>.
erreur_t decode_bloc_acdc(FILE *fichier, uint8_t num_sof, huffman_tree_t *hdc, huffman_tree_t *hac, blocl16_t *sortie, uint8_t s_start, uint8_t s_end, int16_t *dc_prec, uint8_t *off, uint16_t *skip_bloc);
