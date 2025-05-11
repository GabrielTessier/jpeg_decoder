#pragma once

#include <sys/time.h>
#include <vld.h>

// Print sur la sortie standar si option verbose
void print_v(const char *format, ...);

// Print la table de huffman si option verbose
void print_hufftable(huffman_tree_t *tree);

// Transforme la struct timeval en un nombre de microseconde
uint64_t cast_time(struct timeval time);

// Initialise le timer
void init_timer();

// Met le timer à 0
void start_timer();

// Print le text suivi de la valeur du timer (si option timer)
void print_timer(char* text);

// Print une erreur et exit le programme
void erreur(const char *text, ...);

// Affiche le résultat du test <test_name> du fichier <argv[0]> indiqué par
// l'entier <test_var> (1 pour résussi, 0 pour raté)
void test_res(int test_var, char *test_name, char *argv[]);
