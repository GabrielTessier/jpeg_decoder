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

static void decode_bloc_baseline(FILE *fichier, img_t *img, int comp, blocl16_t *sortie, uint8_t s_start, uint8_t s_end, int16_t *dc_prec, uint8_t *off) {
   // On récupère les tables de Huffman et de quantification pour la composante courante
   huffman_tree_t *hdc = NULL;
   huffman_tree_t *hac = NULL;
   qtable_prec_t *qtable = NULL;
   hdc = img->htables->dc[img->comps->comps[comp]->idhdc];
   hac = img->htables->ac[img->comps->comps[comp]->idhac];
   qtable = img->qtables[img->comps->comps[comp]->idq];

   // S'il manque une table on exit avec une erreur
   if (s_start == 0 && hdc == NULL) erreur("Pas de table de huffman DC pour la composante %d\n", comp);
   if (s_end != 0 && hac == NULL) erreur("Pas de table de huffman AC pour la composante %d\n", comp);
   if (qtable == NULL) erreur("Pas de table de quantification pour la composante %d\n", comp);

   // On décode un bloc de l'image (et on chronomètre le temps)
   uint16_t skip_bloc = decode_bloc_acdc(fichier, img->section->num_sof, hdc, hac, sortie, s_start, s_end, dc_prec + comp, off);
   if (skip_bloc != 0) skip_bloc--;
   if (skip_bloc != 0) erreur("Symbole RLE interdit en baseline");
   // On fait la quantification inverse (et on chronomètre le temps)
   iquant(sortie, s_start, s_end, qtable->qtable);
}

void decode_baseline_image(FILE *infile, img_t *img) {
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
   my_timer_t image_timer;
   init_timer(&image_timer);
   start_timer(&image_timer);

   uint8_t y_id, cb_id, cr_id, yhf, yvf, cbhf, cbvf, crhf, crvf;
   uint64_t nb_blocYH, nb_blocCbH, nb_blocCrH;
   char *rgb;
   if (nbcomp == 3) {
      for (uint8_t i = 0; i < nbcomp; i++) {
	 if (img->comps->comps[0]->idc == img->comps->ordre[i]) y_id = i;
	 if (img->comps->comps[1]->idc == img->comps->ordre[i]) cb_id = i;
	 if (img->comps->comps[2]->idc == img->comps->ordre[i]) cr_id = i;
      }
      nb_blocYH = img->nbmcuH * img->comps->comps[y_id]->hsampling;
      nb_blocCbH = img->nbmcuH * img->comps->comps[cb_id]->hsampling;
      nb_blocCrH = img->nbmcuH * img->comps->comps[cr_id]->hsampling;
      yhf = img->max_hsampling / img->comps->comps[y_id]->hsampling;
      yvf = img->max_vsampling / img->comps->comps[y_id]->vsampling;
      cbhf = img->max_hsampling / img->comps->comps[cb_id]->hsampling;
      cbvf = img->max_vsampling / img->comps->comps[cb_id]->vsampling;
      crhf = img->max_hsampling / img->comps->comps[cr_id]->hsampling;
      crvf = img->max_vsampling / img->comps->comps[cr_id]->vsampling;
      rgb = (char *)malloc(sizeof(char) * img->width * 3);
   }

   FILE *outputfile = ouverture_fichier_out(nbcomp, 0);

   // On décode bit par bit, <off> est l'indice du bit dans l'octet en cours de lecture
   if (nbcomp == 1) fprintf(outputfile, "P5\n");
   else if (nbcomp == 3) fprintf(outputfile, "P6\n");
   else erreur("Pas trois composante");
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

   // Tableau contenant les dc précédant le bloc en cours de traitement (initialement 0 pour toutes les composantes)
   int16_t *dc_prec = (int16_t *)calloc(nbcomp, sizeof(int16_t));
   uint8_t off = 0;
   for (uint64_t i = 0; i < img->nbMCU; i++) {
      uint64_t mcuX = i % img->nbmcuH;
      for (uint8_t k = 0; k < nbcomp; k++) {
         uint8_t idcomp = img->comps->ordre[k];
         if (idcomp == 0) break;
         uint8_t indice_comp = 0;
         for (uint8_t c = 0; c < nbcomp; c++) {
            if (img->comps->comps[c]->idc == idcomp) {
               indice_comp = c;
               break;
            }
         }
         uint64_t nbH = img->nbmcuH * img->comps->comps[indice_comp]->hsampling;
         for (uint8_t by = 0; by < img->comps->comps[indice_comp]->vsampling; by++) {
            for (uint8_t bx = 0; bx < img->comps->comps[indice_comp]->hsampling; bx++) {
	      print_v("BLOC %d\n", by * img->comps->comps[indice_comp]->hsampling + bx);
	      uint64_t blocX = mcuX * img->comps->comps[indice_comp]->hsampling + bx;
	      blocl16_t *bloc = (blocl16_t*) calloc(1, sizeof(blocl16_t));
	      decode_bloc_baseline(infile, img, indice_comp, bloc, img->other->ss, img->other->se, dc_prec, &off);
	      bloct16_t *bloc_zz = izz(bloc);
	      free(bloc);
	      bloctu8_t *bloc_idct;
	      if (all_option.idct_fast) bloc_idct = idct_opt(bloc_zz);
	      else bloc_idct = idct(bloc_zz, stockage_coef);
	      free(bloc_zz);
	      if (ycc[k][by * nbH + blocX] != NULL) free(ycc[k][by * nbH + blocX]);
	      ycc[k][by * nbH + blocX] = bloc_idct;
            }
         }
      }
      if (i % img->nbmcuH == img->nbmcuH - 1) { // affichage une ligne de mcu
         save_mcu_ligne(outputfile, img, ycc, rgb, yhf, yvf, y_id, nb_blocYH, cbhf, cbvf, cb_id, nb_blocCbH, crhf, crvf, cr_id, nb_blocCrH);
      }
   }
   fseek(infile, 1, SEEK_CUR);

   print_timer("Décodage complet de l'image", &image_timer);

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
}
