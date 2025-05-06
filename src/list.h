#pragma once

struct maillon_s {
  struct maillon_s *suiv, *prec;
  void* data;
};
typedef struct maillon_s maillon_t;

struct list_s {
  maillon_t *debut, *fin;
};
typedef struct list_s list_t;

list_t *init_list();
void insert_list(list_t *l, void *val);
void *extract_list(list_t *l);
