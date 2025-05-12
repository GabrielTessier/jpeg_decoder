#include <utils.h>
#include <entete.h>
#include <options.h>
#include <stdbool.h>
#include "test_utils.h"

all_option_t all_option;


void test_invader(char *nom_fichier, char *argv[], uint8_t idc, uint8_t idq, uint8_t idhdc, uint8_t idhac) {
   char chemin_fichier[80] = "test/test_file/";
   FILE *fichier = fopen(strcat(chemin_fichier, nom_fichier), "r");
   img_t *img = decode_entete(fichier);

   int test_taille = true;
   int test_qtables = true;
   int test_htables = true;
   int test_comps = true;
   int test_other = true;
   int test_sampling = true;
   int test_mcu = true;

   // TAILLE
   if (img->height != 8) test_taille = false;
   if (img->width != 8) test_taille = false;

   // QTABLES
   for (int i=0; i<4; i++) {
      if (i == idq) {
            if (img->qtables[i] == NULL) {
               test_qtables = false;
            }
            else {
               if (img->qtables[i]->precision != 0) test_qtables = false;
               qtable_t *qtable = img->qtables[i]->qtable;
               uint8_t ref[64] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
               for (int i=0; i<64; i++) {
                  if (qtable->data[i] != ref[i]) test_qtables = false;
               }
            }
      }
      else {
            if (img->qtables[i] != NULL) test_qtables = false;
      }
   }

   // HTABLES
   // DC
   for (int i=0; i<4; i++) {
      if (i == idhdc) {
            if (img->htables->dc[i] == NULL) {
               test_htables = false;
            }
            else {
               if (img->htables->dc[i]->fils[0]->symb != 7) test_htables = false;
            }
      }
      else {
            if (img->htables->dc[i] != NULL) test_htables = false;
      }
   }
   // AC
   for (int i=1; i<4; i++) {
      if (i == idhac) {
            if (img->htables->ac[i] == NULL) {
               test_htables = false;
            }
            else {
               huffman_tree_t *tree = img->htables->ac[i];
               if (tree->fils[0]->fils[0]->symb != 0x17) test_htables = false;
               if (tree->fils[0]->fils[1]->symb != 0x18) test_htables = false;
               if (tree->fils[1]->fils[0]->fils[0]->symb != 0x15) test_htables = false;
               if (tree->fils[1]->fils[0]->fils[1]->fils[0]->symb != 0x8) test_htables = false;
               if (tree->fils[1]->fils[0]->fils[1]->fils[1]->symb != 0x19) test_htables = false;
               if (tree->fils[1]->fils[1]->fils[0]->fils[0]->fils[0]->symb != 0x0) test_htables = false;
               if (tree->fils[1]->fils[1]->fils[0]->fils[0]->fils[1]->symb != 0x9) test_htables = false;
               if (tree->fils[1]->fils[1]->fils[0]->fils[1]->fils[0]->symb != 0x13) test_htables = false;
               if (tree->fils[1]->fils[1]->fils[0]->fils[1]->fils[1]->symb != 0x23) test_htables = false;
               if (tree->fils[1]->fils[1]->fils[1]->fils[0]->fils[0]->symb != 0x28) test_htables = false;
               if (tree->fils[1]->fils[1]->fils[1]->fils[0]->fils[1]->symb != 0x29) test_htables = false;
               if (tree->fils[1]->fils[1]->fils[1]->fils[1]->fils[0]->symb != 0x37) test_htables = false;
            }
      }
      else {
            if (img->htables->ac[i] != NULL) test_htables = false;
      }
   }

   // COMPS
   if (img->comps->nb != 1) test_comps = false;
   if (img->comps->ordre[0] != idc) test_comps = false;
   if (img->comps->ordre[1] != 0) test_comps = false;
   if (img->comps->ordre[2] != 0) test_comps = false;
   if (img->comps->precision_comp != 8) test_comps = false;
   if (img->comps->comps[0]->idc != idc) test_comps = false;
   if (img->comps->comps[0]->hsampling != 1) test_comps = false;
   if (img->comps->comps[0]->vsampling != 1) test_comps = false;
   if (img->comps->comps[0]->idhdc != idhdc) test_comps = false;
   if (img->comps->comps[0]->idhac != idhac) test_comps = false;
   if (img->comps->comps[0]->idq != idq) test_comps = false;
   if (img->comps->comps[1] != NULL) test_comps = false;
   if (img->comps->comps[2] != NULL) test_comps = false;

   // OTHER
   if (strcmp(img->other->jfif,"JFIF") != 0) test_other = false;
   if (img->other->version_jfif_x != 1) test_other = false;
   if (img->other->version_jfif_y != 1) test_other = false;
   if (img->other->ss != 0) test_other = false;
   if (img->other->se != 63) test_other = false;
   if (img->other->ah != 0) test_other = false;
   if (img->other->al != 0) test_other = false;

   // SAMPLING
   if (img->max_hsampling != 1) test_sampling = false;
   if (img->max_vsampling != 1) test_sampling = false;

   // MCU
   if (img->nbmcuH != 1) test_mcu = false;
   if (img->nbmcuV != 1) test_mcu = false;
   if (img->nbMCU != 1) test_mcu = false;

   if (test_taille && test_qtables && test_htables && test_comps && test_other && test_sampling && test_mcu) {
      test_res(true, argv, "Décodage entête : %s", nom_fichier);
   }
   else {
      test_res(test_taille, argv, "Décodage taille : %s", nom_fichier);
      test_res(test_qtables, argv, "Décodage qtables : %s", nom_fichier);
      test_res(test_htables, argv, "Décodage htables : %s", nom_fichier);
      test_res(test_comps, argv, "Décodage comps : %s", nom_fichier);
      test_res(test_other, argv, "Décodage other : %s", nom_fichier);
      test_res(test_sampling, argv, "Décodage sampling max : %s", nom_fichier);
      test_res(test_mcu, argv, "Décodage nb mcu : %s", nom_fichier);
   }
   
   free_img(img);
}


int main(int argc, char *argv[]) {
   (void) argc; // Pour ne pas avoir un warnning unused variable
   test_invader("invader_normal.jpeg", argv, 1, 0, 0, 0);
   test_invader("invader_melange.jpeg", argv, 1, 0, 0, 0);
   test_invader("invader_indice_diff.jpeg", argv, 250, 3, 2, 1);
   return 0;
}
