#pragma once

#include <vld.h>

// Print sur la sortie standar si option verbose
void print_v(const char *format, ...);

// Print la table de huffman si option verbose
void print_hufftable(huffman_tree_t *tree);

// Print une erreur et exit le programme
void erreur(const char *text, ...);


FILE *ouverture_fichier_in();
FILE *ouverture_fichier_out(uint8_t nbcomp, uint8_t nb);
