#pragma once

#include <stdbool.h>


struct maillon_s {
   struct maillon_s *suiv, *prec;
   void* data;
};
typedef struct maillon_s maillon_t;

// Structure implémentant les files
struct file_s {
   maillon_t *debut, *fin;
};
typedef struct file_s file_t;

// Initialise une file
file_t *init_file();

// Alloue un maillon de valeur <*val> et le place au début de <l>
void insertion_file(file_t *l, void *val);

// Libère le dernier maillon de <l> et retourne sa valeur
void *extraction_file(file_t *l);

// Renvoie true si <l> est vide
bool file_vide(file_t *l);
