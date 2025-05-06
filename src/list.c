
#include <stdlib.h>
#include "list.h"

list_t *init_list() {
  list_t* l = (list_t*) malloc(sizeof(list_t));
  l->debut = NULL;
  l->fin = NULL;
  return l;
}


void insert_list(list_t *l, void *data) {
  if (l->debut == NULL) {
    l->debut = (maillon_t*) malloc(sizeof(maillon_t));
    l->debut->suiv = NULL;
    l->debut->prec = NULL;
    l->debut->data = data;
    l->fin = l->debut;
  } else {
    maillon_t *m = (maillon_t*) malloc(sizeof(maillon_t));
    m->data = data;
    m->prec = NULL;
    m->suiv = l->debut;
    l->debut->prec = m;
    l->debut = m;
  }
}

void *extract_list(list_t *l) {
  if (l->fin == NULL) return NULL;
  maillon_t *m = l->fin;
  if (m->prec != NULL) {
    m->prec->suiv = NULL;
    l->fin = m->prec;
  } else {
    l->debut = NULL;
    l->fin = NULL;
  }
  void *data = m->data;
  free(m);
  return data;
}
