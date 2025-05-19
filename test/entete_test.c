#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

#include <utils.h>
#include <entete.h>
#include <options.h>
#include <img.h>
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

// test de décodage de l'entête sur les fichiers <noms_fichiers>.
// La fonction <decode_entete> est censée échouer sur ces fichiers.
// Dans le cas où <decode_entete> finit, le test échoue et on affiche les
// résultats des test sur chaque fichier.
// Sinon on affiche juste <test_name>.
static void test_fail(char *noms_fichiers[], erreur_code_t err_codes[], int nb_fichiers, char *test_name, char *argv[]);

   

static void test_invader(char *nom_fichier, char *argv[], uint8_t idc, uint8_t idq, uint8_t idhdc, uint8_t idhac) {
   char chemin_fichier[80] = "test/test_file/";
   FILE *fichier = fopen(strcat(chemin_fichier, nom_fichier), "r");

   img_t *img = init_img();
   erreur_t err = decode_entete(fichier, true, img);

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
   test_htables = parse_comp_hufftables_blabla(nom_fichier, *img->htables);

   // COMPS
   if (img->comps->nb != 1)                  test_comps = false;
   if (img->comps->ordre[0] != idc)          test_comps = false;
   if (img->comps->ordre[1] != 0)            test_comps = false;
   if (img->comps->ordre[2] != 0)            test_comps = false;
   if (img->comps->precision_comp != 8)      test_comps = false;
   if (img->comps->comps[0]->idc != idc)     test_comps = false;
   if (img->comps->comps[0]->hsampling != 1) test_comps = false;
   if (img->comps->comps[0]->vsampling != 1) test_comps = false;
   if (img->comps->comps[0]->idhdc != idhdc) test_comps = false;
   if (img->comps->comps[0]->idhac != idhac) test_comps = false;
   if (img->comps->comps[0]->idq != idq)     test_comps = false;
   if (img->comps->comps[1] != NULL)         test_comps = false;
   if (img->comps->comps[2] != NULL)         test_comps = false;

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
      test_res(test_taille   , argv, "Décodage taille : %s (%d)",	nom_fichier, err.code);
      test_res(test_qtables  , argv, "Décodage qtables : %s (%d)", 	nom_fichier, err.code);
      test_res(test_htables  , argv, "Décodage htables : %s (%d)", 	nom_fichier, err.code);
      test_res(test_comps    , argv, "Décodage comps : %s (%d)", 	nom_fichier, err.code);
      test_res(test_other    , argv, "Décodage other : %s (%d)",        nom_fichier, err.code);
      test_res(test_sampling , argv, "Décodage sampling max : %s (%d)", nom_fichier, err.code);
      test_res(test_mcu      , argv, "Décodage nb mcu : %s (%d)",  	nom_fichier, err.code);
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
   char str[200];
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
   free(line);
   return test_hufftable;
}

static void test_shaun(char *nom_fichier, char *argv[]) {
   char chemin_fichier[80] = "test/test_file/";
   FILE *fichier = fopen(strcat(chemin_fichier, nom_fichier), "r");
   img_t *img = init_img();
   decode_entete(fichier, true, img);

   // Variables de test
   bool test_taille   = true;
   bool test_qtables  = true;
   bool test_htables  = true;
   bool test_comps    = true;
   bool test_other    = true;
   bool test_sampling = true;
   bool test_mcu      = true;

   // TAILLE
   if (img->height != 225) test_taille = false;
   if (img->width != 300) test_taille = false;

   // QTABLES
   // tables de référence obtenues avec hexdump
   uint8_t qt_ref[2][64] = {
      {0x0a, 0x07, 0x07, 0x09, 0x07, 0x06, 0x0a, 0x09,
       0x08, 0x09, 0x0b, 0x0b, 0x0a, 0x0c, 0x0f, 0x19,
       0x10, 0x0f, 0x0e, 0x0e, 0x0f, 0x1e, 0x16, 0x17,
       0x12, 0x19, 0x24, 0x20, 0x26, 0x25, 0x23, 0x20,
       0x23, 0x22, 0x28, 0x2d, 0x39, 0x30, 0x28, 0x2a,
       0x36, 0x2b, 0x22, 0x23, 0x32, 0x44, 0x32, 0x36,
       0x3b, 0x3d, 0x40, 0x40, 0x40, 0x26, 0x30, 0x46,
       0x4b, 0x45, 0x3e, 0x4a, 0x39, 0x3f, 0x40, 0x3d},
      {0x0b, 0x0b, 0x0b, 0x0f, 0x0d, 0x0f, 0x1d, 0x10,
       0x10, 0x1d, 0x3d, 0x29, 0x23, 0x29, 0x3d, 0x3d,
       0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d,
       0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d,
       0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d,
       0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d,
       0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d,
       0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d}
   };
   for (int i=0; i<4; i++) {
      if (i == 2 || i == 3) { // tables de quantification non fournies par le fichier
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
   if (img->comps->nb != 3)                  test_comps = false;
   if (img->comps->ordre[0] != 1)            test_comps = false;
   if (img->comps->ordre[1] != 2)            test_comps = false;
   if (img->comps->ordre[2] != 3)            test_comps = false;
   if (img->comps->precision_comp != 8)      test_comps = false;
  
   if (img->comps->comps[0]->idc != 1)       test_comps = false;
   if (img->comps->comps[0]->hsampling != 2) test_comps = false;
   if (img->comps->comps[0]->vsampling != 2) test_comps = false;
   if (img->comps->comps[0]->idhdc != 0)     test_comps = false;
   if (img->comps->comps[0]->idhac != 0)     test_comps = false;
   if (img->comps->comps[0]->idq != 0)       test_comps = false;
 
   if (img->comps->comps[1]->idc != 2)       test_comps = false;
   if (img->comps->comps[1]->hsampling != 1) test_comps = false;
   if (img->comps->comps[1]->vsampling != 1) test_comps = false;
   if (img->comps->comps[1]->idhdc != 1)     test_comps = false;
   if (img->comps->comps[1]->idhac != 1)     test_comps = false;
   if (img->comps->comps[1]->idq != 1)       test_comps = false;

   if (img->comps->comps[2]->idc != 3)       test_comps = false;
   if (img->comps->comps[2]->hsampling != 1) test_comps = false;
   if (img->comps->comps[2]->vsampling != 1) test_comps = false;
   if (img->comps->comps[2]->idhdc != 1)     test_comps = false;
   if (img->comps->comps[2]->idhac != 1)     test_comps = false;
   if (img->comps->comps[2]->idq != 1)       test_comps = false;

   // OTHER
   if (strcmp(img->other->jfif,"JFIF") != 0) test_other = false;
   if (img->other->version_jfif_x != 1)      test_other = false;
   if (img->other->version_jfif_y != 1)      test_other = false;
   if (img->other->ss != 0)                  test_other = false;
   if (img->other->se != 63)                 test_other = false;
   if (img->other->ah != 0)                  test_other = false;
   if (img->other->al != 0)                  test_other = false;

   // SAMPLING
   if (img->max_hsampling != 2)              test_sampling = false;
   if (img->max_vsampling != 2)              test_sampling = false;

   // MCU
   if (img->nbmcuH != 19)                    test_mcu = false;
   if (img->nbmcuV != 15)                    test_mcu = false;
   if (img->nbMCU != 19*15)                  test_mcu = false;

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

static void test_fail(char *noms_fichiers[], erreur_code_t err_codes[], int nb_fichiers, char *test_name, char *argv[]) {
   erreur_t *res_err = (erreur_t *) malloc(sizeof(erreur_t)*nb_fichiers);
   for (int i=0; i<nb_fichiers; i++) {
     char *chemin_fichier = (char *) calloc(16+strlen(noms_fichiers[i]), sizeof(char));
      strcat(chemin_fichier, "test/test_file/");
      FILE *fichier = fopen(strcat(chemin_fichier, noms_fichiers[i]), "r");
      img_t *img = init_img();
      erreur_t err = decode_entete(fichier, true, img);
      res_err[i] = err;
      free(chemin_fichier);
      free_img(img);
   }
   
   bool test_all = true;
   for (int i=0; i<nb_fichiers; i++) {
      if (res_err[i].code != err_codes[i]) test_all = false;
   }
   if (test_all) {
      test_res(test_all, argv, "%s", test_name);
   } else {
      for (int i=0; i<nb_fichiers; i++) {
	 erreur_code_t code = res_err[i].code;
	 if (code != err_codes[i]) {
	    test_res(res_err[i].code == err_codes[i], argv, "entete invalide : %s (%d: %s)", noms_fichiers[i], res_err[i].code, res_err[i].com);
	 } else {
	    test_res(res_err[i].code == err_codes[i], argv, "entete invalide : %s (%d)", noms_fichiers[i], res_err[i].code);
	 }
      }
   }
   free(res_err);
}


int main(int argc, char *argv[]) {
   (void) argc; // Pour ne pas avoir un warning unused variable à la compilation
   test_invader("invader_normal.jpeg", argv, 1, 0, 0, 0);
   test_invader("invader_melange.jpeg", argv, 1, 0, 0, 0);
   test_invader("invader_indice_diff.jpeg", argv, 250, 3, 0, 1);
   test_shaun("shaun_the_sheep.jpeg", argv);

   // formatage de l'entête non supporté par la norme
   char *noms_fichiers_jfif[] = {"invader_bad_entete_jfif.jpeg",
				 "invader_bad_entete_vjfif0.jpeg",
				 "invader_bad_entete_vjfif1.jpeg"};
   erreur_code_t err_codes_jfif[] = {ERR_NO_JFIF, ERR_JFIF_VERSION, ERR_JFIF_VERSION};
   
   char *noms_fichiers_sof0[] = {"invader_bad_entete_sof0_p.jpeg"};
   erreur_code_t err_codes_sof0[] = {ERR_SOF_PRECISION};
   
   char *noms_fichiers_dqt[]  = {"invader_bad_entete_dqt_p.jpeg"};
   erreur_code_t err_codes_dqt[] = {ERR_DQT_PRECISION};
   
   char *noms_fichiers_dht[]  = {"invader_bad_entete_dht_dc2.jpeg",
				 "invader_bad_entete_dht_dc3.jpeg",
				 "invader_bad_entete_dht_ac2.jpeg",
				 "invader_bad_entete_dht_ac3.jpeg"};
   erreur_code_t err_codes_dht[] = {ERR_HUFF_ID, ERR_HUFF_ID, ERR_HUFF_ID, ERR_HUFF_ID};

   char *noms_fichiers_sos[] = {"invader_bad_entete_baseline_sos_idht_dc.jpeg",
				"invader_bad_entete_baseline_sos_idht_ac.jpeg",
   				"invader_bad_entete_baseline_sos_ss.jpeg",
				"invader_bad_entete_baseline_sos_se.jpeg",
				"invader_bad_entete_baseline_sos_ah.jpeg",
				"invader_bad_entete_baseline_sos_al.jpeg"};
   erreur_code_t err_codes_sos[] = {ERR_HUFF_ID, ERR_HUFF_ID, ERR_SOS_SS, ERR_SOS_SE, ERR_SOS_AH, ERR_SOS_AL};

   char *noms_fichiers_sof2[] = {"invader_bad_entete_prog_sof2_p.jpg",
				 "invader_bad_entete_prog_sof2_idcomp.jpg",
				 "invader_bad_entete_prog_sos_ss.jpg",
				 "invader_bad_entete_prog_sos_se.jpg",
				 "invader_bad_entete_prog_sos_ah.jpg",
				 "invader_bad_entete_prog_sos_al.jpg"};
   erreur_code_t err_codes_sof2[] = {ERR_SOF_PRECISION, ERR_SOS_COMP_ID, ERR_SOS_SS, ERR_SOS_SE, ERR_SOS_AH, ERR_SOS_AL};

   char *noms_fichiers_eoi[] = {"invader_bad_entete_eoi_av_sos.jpeg",
				"invader_bad_entete_no_eoi.jpeg"};
   erreur_code_t err_codes_eoi[] = {ERR_EOI_BEFORE_SOS, ERR_NO_EOI};

   char *noms_fichiers_soi[] = {"invader_bad_entete_no_soi.jpeg",
				"invader_bad_entete_sev_soi.jpeg"};
   erreur_code_t err_codes_soi[] = {ERR_NO_SOI, ERR_SEVERAL_SOI};
      
   
   test_fail(noms_fichiers_jfif, err_codes_jfif, 3, "entête invalide : jfif",		argv);
   test_fail(noms_fichiers_sof0, err_codes_sof0, 1, "entête invalide : précision SOF0", argv);
   test_fail(noms_fichiers_dqt,  err_codes_dqt,  1, "entête invalide : précision DQT",  argv);
   test_fail(noms_fichiers_dht,  err_codes_dht,  4, "entête invalide : indices DHT",  	argv);
   test_fail(noms_fichiers_sos,  err_codes_sos,  6, "entête invalide : section SOS",  	argv);
   test_fail(noms_fichiers_sof2, err_codes_sof2, 6, "entête invalide : progressif",  	argv);
   test_fail(noms_fichiers_eoi,  err_codes_eoi,  2, "entête invalide : EOI",  		argv);
   test_fail(noms_fichiers_soi,  err_codes_soi,  2, "entête invalide : SOI",  		argv);
   return 0;
}
