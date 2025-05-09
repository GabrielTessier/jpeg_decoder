#pragma once

#include <stdbool.h>

struct maillon_s {
  struct maillon_s *suiv, *prec;
  void* data;
};
typedef struct maillon_s maillon_t;

struct file_s {
  maillon_t *debut, *fin;
};
typedef struct file_s file_t;

file_t *init_file();
void insertion_file(file_t *l, void *val);
void *extraction_file(file_t *l);
bool file_vide(file_t *l);
