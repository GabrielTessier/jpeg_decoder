#pragma once

#include "erreur.h"
#include <vld.h>

// Print sur la sortie standar si option verbose
void print_v(const char *format, ...);

// Print la table de huffman si option verbose
void print_hufftable(huffman_tree_t *tree);

erreur_t ouverture_fichier_in(FILE **fichier);
char *out_file_name(uint8_t nbcomp, uint8_t nb);
FILE *ouverture_fichier_out(uint8_t nbcomp, uint8_t nb);
