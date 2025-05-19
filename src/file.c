#include <stdbool.h>
#include <stdlib.h>

#include <file.h>


file_t *init_file() {
   file_t* l = (file_t*) malloc(sizeof(file_t));
   l->debut = NULL;
   l->fin = NULL;
   return l;
}

void insertion_file(file_t *l, void *data) {
   if (file_vide(l)) {
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

void *extraction_file(file_t *l) {
   if (file_vide(l)) return NULL;
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

bool file_vide(file_t *l) {
   return l->debut == NULL;
}
