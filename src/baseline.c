#include "jpeg2ppm.h"
#include <stdio.h>
#include <stdlib.h>

#include <utils.h>
#include <entete.h>
#include <timer.h>
#include <idct.h>
#include <idct_opt.h>
#include <options.h>
#include <ycc2rgb.h>
#include <decoder_utils.h>
#include <baseline.h>

extern all_option_t all_option;

static erreur_t decode_bloc_baseline(FILE *fichier, img_t *img, int comp, blocl16_t *sortie, int16_t *dc_prec, uint8_t *off) {
   // On récupère les tables de Huffman et de quantification pour la composante courante
   huffman_tree_t *hdc = NULL;
   huffman_tree_t *hac = NULL;
   qtable_prec_t *qtable = NULL;
   hdc = img->htables->dc[img->comps->comps[comp]->idhdc];
   hac = img->htables->ac[img->comps->comps[comp]->idhac];
   qtable = img->qtables[img->comps->comps[comp]->idq];

   // S'il manque une table on exit avec une erreur
   if (hdc == NULL) {
      char *str = malloc(80);
      sprintf(str, "Pas de table de huffman DC pour la composante %d", comp);
      return (erreur_t) {.code = ERR_NO_HT, .com = str};
   }
   if (hac == NULL) {
      char *str = malloc(80);
      sprintf(str, "Pas de table de huffman AC pour la composante %d", comp);
      return (erreur_t) {.code = ERR_NO_HT, .com = str};
   }
   if (qtable == NULL) {
      char *str = malloc(80);
      sprintf(str, "Pas de table de quantification pour la composante %d", comp);
      return (erreur_t) {.code = ERR_NO_HT, .com = str};
   }

   // On décode un bloc de l'image (et on chronomètre le temps)
   uint16_t skip_bloc;
   erreur_t err = decode_bloc_acdc(fichier, img, hdc, hac, sortie, dc_prec + comp, off, &skip_bloc);
   if (err.code) return err;
   if (skip_bloc > 1) {
      return (erreur_t) {.code = ERR_AC_BAD, .com = "Symbole RLE interdit en baseline"};
   }
   
   // On fait la quantification inverse (et on chronomètre le temps)
   iquant(sortie, img->other->ss, img->other->se, qtable->qtable);
   
   return (erreur_t) {.code = SUCCESS};
}

static void get_ycc_info(img_t *img, uint8_t *y_id, uint8_t *cb_id, uint8_t *cr_id, uint8_t *yhf, uint8_t *yvf, uint8_t *cbhf, uint8_t *cbvf, uint8_t *crhf, uint8_t *crvf, uint64_t *nb_blocYH, uint64_t *nb_blocCbH, uint64_t *nb_blocCrH, char **rgb) {
   if (img->comps->nb == 3) {
      for (uint8_t i = 0; i < img->comps->nb; i++) {
	 if (img->comps->comps[0]->idc == img->comps->ordre[i]) *y_id = i;
	 if (img->comps->comps[1]->idc == img->comps->ordre[i]) *cb_id = i;
	 if (img->comps->comps[2]->idc == img->comps->ordre[i]) *cr_id = i;
      }
      *nb_blocYH = img->nbmcuH * img->comps->comps[*y_id]->hsampling;
      *nb_blocCbH = img->nbmcuH * img->comps->comps[*cb_id]->hsampling;
      *nb_blocCrH = img->nbmcuH * img->comps->comps[*cr_id]->hsampling;
      *yhf = img->max_hsampling / img->comps->comps[*y_id]->hsampling;
      *yvf = img->max_vsampling / img->comps->comps[*y_id]->vsampling;
      *cbhf = img->max_hsampling / img->comps->comps[*cb_id]->hsampling;
      *cbvf = img->max_vsampling / img->comps->comps[*cb_id]->vsampling;
      *crhf = img->max_hsampling / img->comps->comps[*cr_id]->hsampling;
      *crvf = img->max_vsampling / img->comps->comps[*cr_id]->vsampling;
      *rgb = (char *)malloc(sizeof(char) * img->width * 3);
   }
}

erreur_t decode_baseline_image(FILE *infile, img_t *img) {
   print_huffman_quant_table(img);
   
   uint8_t nbcomp = img->comps->nb;
   // Calcul des coefficients pour la DCT inverse (lente)
   float stockage_coef[8][8][8][8];
   if (!all_option.idct_fast) {
      my_timer_t coef_idct;
      init_timer(&coef_idct);
      start_timer(&coef_idct);
      calc_coef(stockage_coef);
      print_timer("Calcul des coefficients de l'iDCT", &coef_idct);
   }

   // Décodage de l'image
   my_timer_t timer_image;
   init_timer(&timer_image);
   start_timer(&timer_image);

   uint8_t y_id, cb_id, cr_id, yhf, yvf, cbhf, cbvf, crhf, crvf;
   uint64_t nb_blocYH, nb_blocCbH, nb_blocCrH;
   char *rgb;
   if (nbcomp == 3) get_ycc_info(img, &y_id, &cb_id, &cr_id, &yhf, &yvf, &cbhf, &cbvf, &crhf, &crvf, &nb_blocYH, &nb_blocCbH, &nb_blocCrH, &rgb);

   FILE *outputfile = ouverture_fichier_out(nbcomp, 0);

   // On décode bit par bit, <off> est l'indice du bit dans l'octet en cours de lecture
   if (nbcomp == 1) {
      fprintf(outputfile, "P5\n");
   } else if (nbcomp == 3) {
      fprintf(outputfile, "P6\n");
   } else {
      return (erreur_t) {.code = ERR_NB_COMP, .com = "Il faut une ou trois composante"};
   }
   fprintf(outputfile, "%d %d\n", img->width, img->height); // largeur, hateur
   fprintf(outputfile, "255\n");

   // Initialisation du tableau ycc :
   // ycc contient un tableau par composante
   // Chaque sous-tableau est de taille le nombre de bloc dans la composante (et stocke des blocs 8x8)
   bloctu8_t ***ycc = (bloctu8_t ***)malloc(sizeof(bloctu8_t **) * nbcomp);
   for (uint8_t i = 0; i < nbcomp; i++) {
      uint64_t nbH = img->nbmcuH * img->comps->comps[i]->hsampling;
      uint64_t nbV = img->comps->comps[i]->vsampling;
      ycc[i] = (bloctu8_t **)calloc(nbH * nbV, sizeof(bloctu8_t *));
   }

   // Initialisation des chronomètres
   my_timer_t timer_decode_bloc;
   init_timer(&timer_decode_bloc);
   my_timer_t timer_izz;
   init_timer(&timer_izz);
   my_timer_t timer_idct;
   init_timer(&timer_idct);
   my_timer_t timer_affichage;
   init_timer(&timer_affichage);
   

   // Tableau contenant les dc précédant le bloc en cours de traitement (initialement 0 pour toutes les composantes)
   int16_t *dc_prec = (int16_t *)calloc(nbcomp, sizeof(int16_t));
   uint8_t off = 0;
   for (uint64_t i = 0; i < img->nbMCU; i++) {
      uint64_t mcuX = i % img->nbmcuH;
      for (uint8_t k = 0; k < nbcomp; k++) {
         int16_t indice_comp = get_composante(img, k);
	 if (indice_comp == -1) break;   // Si un scan ne contient pas toutes les composantes
	 
         uint64_t nbH = img->nbmcuH * img->comps->comps[indice_comp]->hsampling;
         for (uint8_t by = 0; by < img->comps->comps[indice_comp]->vsampling; by++) {
            for (uint8_t bx = 0; bx < img->comps->comps[indice_comp]->hsampling; bx++) {
               print_v("BLOC %d\n", by * img->comps->comps[indice_comp]->hsampling + bx);
               uint64_t blocX = mcuX * img->comps->comps[indice_comp]->hsampling + bx;
               blocl16_t *bloc = (blocl16_t*) calloc(1, sizeof(blocl16_t));
               
               start_timer(&timer_decode_bloc);
               erreur_t err = decode_bloc_baseline(infile, img, indice_comp, bloc, dc_prec, &off);
               stop_timer(&timer_decode_bloc);
               if (err.code) return err;

               start_timer(&timer_izz);
               bloct16_t *bloc_zz = izz(bloc);
               stop_timer(&timer_izz);
               free(bloc);

               bloctu8_t *bloc_idct;
               start_timer(&timer_idct);
               if (all_option.idct_fast) bloc_idct = idct_opt(bloc_zz);
               else bloc_idct = idct(bloc_zz, stockage_coef);
               stop_timer(&timer_idct);

               free(bloc_zz);
               if (ycc[k][by * nbH + blocX] != NULL) free(ycc[k][by * nbH + blocX]);
               ycc[k][by * nbH + blocX] = bloc_idct;
            }
         }
      }
      if (i % img->nbmcuH == img->nbmcuH - 1) { // affichage une ligne de mcu
         stop_timer(&timer_image);
         start_timer(&timer_affichage);
         if (nbcomp == 1) {
            save_mcu_ligne_bw(outputfile, img, ycc);
         } else if (nbcomp == 3) {
            save_mcu_ligne_color(outputfile, img, ycc, rgb, yhf, yvf, y_id, nb_blocYH, cbhf, cbvf, cb_id, nb_blocCbH, crhf, crvf, cr_id, nb_blocCrH);
         }
         stop_timer(&timer_affichage);
         start_timer(&timer_image);
      }
   }
   fseek(infile, 1, SEEK_CUR);

   print_timer("Décodage DC/AC et Quantification", &timer_decode_bloc);
   print_timer("IZZ", &timer_izz);
   print_timer("IDCT", &timer_idct);
   print_timer("Décodage complet de l'image", &timer_image);
   print_timer("Affichage image", &timer_affichage);

   fclose(outputfile);
   free(dc_prec);
   // Free ycc
   for (uint8_t i = 0; i < nbcomp; i++) {
      uint64_t nbH = img->nbmcuH * img->comps->comps[i]->hsampling;
      uint64_t nbV = img->comps->comps[i]->vsampling;
      for (uint64_t j = 0; j < nbH * nbV; j++) {
         free(ycc[i][j]);
      }
      free(ycc[i]);
   }
   free(ycc);

   if (nbcomp == 3) free(rgb);

   return (erreur_t) {.code = SUCCESS};
}
