#include <stdlib.h>

#include <img.h>


// Libère les tables de quantification
static void free_qtables(qtable_prec_t **qtables);

// Libère les tables de Huffman
static void free_htables(htables_t *htables);

// Libère les informations sur les composantes
static void free_comps(comps_t *comps);

// Libère les informations sur l'avancement du traitement de l'entête
static void free_section_done(section_done_t *section);

// Libère les informations complémentaires
static void free_other(other_t *other);

// Libère les commentaires de l'image
static void free_commentaires(com_t *com);



static void free_qtables(qtable_prec_t **qtables) {
   for (int i=0; i<4; i++) {
      if (qtables[i] != NULL) {
         free(qtables[i]->qtable);
         free(qtables[i]);
      }
   }
}

void free_huffman_tree(huffman_tree_t *tree) {
   if (tree == NULL) return;
   free_huffman_tree(tree->fils[0]);
   free_huffman_tree(tree->fils[1]);
   free(tree);
}

static void free_htables(htables_t *htables) {
   for (int i=0; i<4; i++) {
      if (htables->ac[i] != NULL) free_huffman_tree(htables->ac[i]);
      if (htables->dc[i] != NULL) free_huffman_tree(htables->dc[i]);
   }
   free(htables);
}

static void free_comps(comps_t *comps) {
   for (int i=0; i<3; i++) {
      if (comps->comps[i] != NULL) free(comps->comps[i]);
   }
   free(comps);
}

static void free_section_done(section_done_t *section) {
   free(section);
}

static void free_other(other_t *other) {
   free(other);
}

static void free_commentaires(com_t *com) {
   for (int i=0; i<com->nb; i++) {
      free(com->com[i]);
   }
   free(com->com);
   free(com);
}

void free_img(img_t *img) {
   free_qtables(img->qtables);
   free_htables(img->htables);
   free_comps(img->comps);
   free_section_done(img->section);
   free_other(img->other);
   free_commentaires(img->com);
   free(img);
}

img_t* init_img() {
   img_t *img    = calloc(1,sizeof(img_t));
   img->htables  = calloc(1,sizeof(htables_t));
   img->comps    = calloc(1,sizeof(comps_t));
   img->section  = calloc(1,sizeof(section_done_t));
   img->other    = calloc(1,sizeof(other_t));
   img->com      = calloc(1,sizeof(com_t));
   return img;
}
