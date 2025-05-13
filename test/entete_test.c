#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <utils.h>
#include <entete.h>
#include <options.h>
#include <vld.h>
#include "test_utils.h"


// test de décodage de l'entête sur les fichiers invader.
static void test_invader(char *nom_fichier, char *argv[], uint8_t idc, uint8_t idq, uint8_t idhdc, uint8_t idhac);

// descend dans l'arbre de Huffman <ht> en suivant le chemin <path>.
static uint8_t parcours_hufftree(huffman_tree_t ht, char *path);

// parse le décodage des arbres Huffman par l'exécutable de référence
// jpeg2blabla et les compare avec les tables de <hts>
static bool parse_comp_hufftables_blabla(char *nom_fichier, htables_t hts);

// test de décodage de l'entête sur shaun_the_sheep.
static void test_shaun(char *nom_fichier, char *argv[]);


static void test_invader(char *nom_fichier, char *argv[], uint8_t idc, uint8_t idq, uint8_t idhdc, uint8_t idhac) {
   char chemin_fichier[80] = "test/test_file/";
   FILE *fichier = fopen(strcat(chemin_fichier, nom_fichier), "r");

   img_t *img = init_img();
   decode_entete(fichier, true, img);

   // Variables de test
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

static uint8_t parcours_hufftree(huffman_tree_t ht, char *path) {
   int i = 0;
   while(path[i]) {
      ht = *ht.fils[path[i] - '0'];
   }
   return ht.symb;
}

static bool parse_comp_hufftables_blabla(char *nom_fichier, htables_t hts) {
   char str[100];
   sprintf(str, "./bin/jpeg2blabla %s > /tmp/blabla_output.txt", nom_fichier);
   system(str);
   FILE *file = fopen("/tmp/blabla_output.txt", "r");
   size_t line_size_max = 100;
   char *line = (char *) malloc(sizeof(char)*line_size_max);
   bool test_hufftable = true;
   while (fgets(line, line_size_max, file)) {
      if (strstr(line, "Huffman table type ") != NULL) {
	 // type de table
	 char acdc[3];
	 sscanf(line, "Huffman table type %s\n", acdc);
	 fgets(line, line_size_max, file);
	 // indice de table
	 int id;
	 sscanf(line, "Huffman table index %d\n", &id);
	 fgets(line, line_size_max, file);
	 // nombre de symboles de Huffman
	 int nb_symb;
	 sscanf(line, "total nb of Huffman symbols %d\n", &nb_symb);

	 huffman_tree_t *ht;
	 if (strcmp(acdc, "AC") == 0) {
	    ht = *hts.ac;
	 } else if (strcmp(acdc, "DC") == 0) {
	    ht = *hts.dc;
      	 } else {
	    erreur("table de huffman ni AC ni DC");
      	 }
	 for (int i=0; i<nb_symb; i++) {
	    char code[80];
	    uint8_t symb;
	    sscanf(line, "path: %s symbol: %c\n", code, &symb);
	    if (symb != parcours_hufftree(ht[id], code)) {
	       test_hufftable = false;
	    }
	 }
      }
   }
   return test_hufftable;
}

static void test_shaun(char *nom_fichier, char *argv[]) {
   char chemin_fichier[80] = "test/test_file/";
   FILE *fichier = fopen(strcat(chemin_fichier, nom_fichier), "r");
   img_t *img = init_img();
   decode_entete(fichier, true, img);

   // Variables de test
   int test_taille	= true;
   int test_qtables	= true;
   int test_htables	= true;
   int test_comps	= true;
   int test_other	= true;
   int test_sampling	= true;
   int test_mcu		= true;

   // TAILLE
   if (img->height != 8) test_taille = false;
   if (img->width != 8) test_taille = false;

   // QTABLES
   uint8_t qt_ref[2][64] = {
      {
	 0x0a, 0x07, 0x07, 0x09, 0x07, 0x06, 0x0a, 0x09,
	 0x08, 0x09, 0x0b, 0x0b, 0x0a, 0x0c, 0x0f, 0x19,
	 0x10, 0x0f, 0x0e, 0x0e, 0x0f, 0x1e, 0x16, 0x17,
	 0x12, 0x19, 0x24, 0x20, 0x26, 0x25, 0x23, 0x20,
	 0x23, 0x22, 0x28, 0x2d, 0x39, 0x30, 0x28, 0x2a,
	 0x36, 0x2b, 0x22, 0x23, 0x32, 0x44, 0x32, 0x36,
	 0x3b, 0x3d, 0x40, 0x40, 0x40, 0x26, 0x30, 0x46,
	 0x4b, 0x45, 0x3e, 0x4a, 0x39, 0x3f, 0x40, 0x3d
      },
      {
	 0x0b, 0x0b, 0x0b, 0x0f, 0x0d, 0x0f, 0x1d, 0x10,
	 0x10, 0x1d, 0x3d, 0x29, 0x23, 0x29, 0x3d, 0x3d,
	 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d,
	 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d,
	 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d,
	 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0xff, 0xc0,
	 0x00, 0x11, 0x08, 0x00, 0xe1, 0x01, 0x2c, 0x03,
	 0x01, 0x22, 0x00, 0x02, 0x11, 0x01, 0x03, 0x11
      }};
   for (int i=0; i<4; i++) {
      if (i == 2 || i == 3) { // tables de quantification non fournies par l'image
	 if (img->qtables[i] != NULL) {
	    test_qtables = false;
	 }
      } else {
	 for (int j=0; j<64; j++) {
	    if (img->qtables[i]->qtable->data[j] != qt_ref[i][j]) {
	       test_qtables = false;
	    }
	 }
      }
   }

   // HTABLES
   test_htables = parse_comp_hufftables_blabla(nom_fichier, *img->htables);

   // COMPS
   if (img->comps->nb != 3)			test_comps = false;
   if (img->comps->ordre[0] != 1)		test_comps = false;
   if (img->comps->ordre[1] != 2)		test_comps = false;
   if (img->comps->ordre[2] != 3)		test_comps = false;
   if (img->comps->precision_comp != 8)		test_comps = false;
   
   if (img->comps->comps[0]->idc != 1)		test_comps = false;
   if (img->comps->comps[0]->hsampling != 2)	test_comps = false;
   if (img->comps->comps[0]->vsampling != 2)	test_comps = false;
   if (img->comps->comps[0]->idhdc != 0)	test_comps = false;
   if (img->comps->comps[0]->idhac != 0)	test_comps = false;
   if (img->comps->comps[0]->idq != 0)		test_comps = false;

   if (img->comps->comps[1]->idc != 2)		test_comps = false;
   if (img->comps->comps[0]->hsampling != 1)	test_comps = false;
   if (img->comps->comps[0]->vsampling != 1)	test_comps = false;
   if (img->comps->comps[0]->idhdc != 1)	test_comps = false;
   if (img->comps->comps[0]->idhac != 1)	test_comps = false;
   if (img->comps->comps[0]->idq != 1)		test_comps = false;

   if (img->comps->comps[2]->idc != 3)		test_comps = false;
   if (img->comps->comps[0]->hsampling != 1)	test_comps = false;
   if (img->comps->comps[0]->vsampling != 1)	test_comps = false;
   if (img->comps->comps[0]->idhdc != 2)	test_comps = false;
   if (img->comps->comps[0]->idhac != 2)	test_comps = false;
   if (img->comps->comps[0]->idq != 2)		test_comps = false;

   // OTHER
   if (strcmp(img->other->jfif,"JFIF") != 0)	test_other = false;
   if (img->other->version_jfif_x != 1)		test_other = false;
   if (img->other->version_jfif_y != 1)		test_other = false;
   if (img->other->ss != 0)			test_other = false;
   if (img->other->se != 63)			test_other = false;
   if (img->other->ah != 0)			test_other = false;
   if (img->other->al != 0)			test_other = false;

   // SAMPLING
   if (img->max_hsampling != 1)			test_sampling = false;
   if (img->max_vsampling != 1)			test_sampling = false;

   // MCU
   if (img->nbmcuH != 1)			test_mcu = false;
   if (img->nbmcuV != 1)			test_mcu = false;
   if (img->nbMCU != 1)				test_mcu = false;

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
   (void) argc; // Pour ne pas avoir un warning unused variable à la compilation
   test_invader("invader_normal.jpeg", argv, 1, 0, 0, 0);
   test_invader("invader_melange.jpeg", argv, 1, 0, 0, 0);
   test_invader("invader_indice_diff.jpeg", argv, 250, 3, 0, 1);
   test_shaun("shaun_the_sheep.jpeg", argv);
   return 0;
}
