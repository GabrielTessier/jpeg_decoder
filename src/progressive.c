
#include <utils.h>
#include <options.h>
#include <idct.h>
#include <idct_opt.h>
#include <vld.h>
#include <iqzz.h>
#include <decoder_utils.h>
#include <progressive.h>

extern all_option_t all_option;
extern bool stop;

static uint16_t decode_bloc_progressive(FILE *fichier, img_t *img, int comp, blocl16_t *sortie, int16_t *dc_prec, uint8_t *off) {
   uint8_t s_start = img->other->ss;
   uint8_t s_end = img->other->se;
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
   if (stop) return 0;
   if (skip_bloc != 0) skip_bloc--;
   // On fait la quantification inverse (et on chronomètre le temps)
   iquant(sortie, s_start, s_end, qtable->qtable);

   return skip_bloc;
}

static blocl16_t ***init_sortieq(img_t *img){
   blocl16_t ***sortieq = (blocl16_t ***)malloc(sizeof(blocl16_t **) * img->comps->nb);
   for (uint8_t i = 0; i < img->comps->nb; i++) {
      uint64_t nbH = img->nbmcuH * img->comps->comps[i]->hsampling;
      uint64_t nbV = img->nbmcuV * img->comps->comps[i]->vsampling;
      sortieq[i] = (blocl16_t **)malloc(nbH * nbV * sizeof(blocl16_t));
      for (uint64_t j = 0; j < nbH * nbV; j++) {
         sortieq[i][j] = (blocl16_t *)calloc(1, sizeof(blocl16_t));
      }
   }
   return sortieq;
}

void decode_progressive_image(FILE *infile, img_t *img) {
   uint8_t nbcomp = img->comps->nb;
   blocl16_t ***sortieq = init_sortieq(img);
   // Calcul des coefficients pour la DCT inverse (lente)
   float stockage_coef[8][8][8][8];
   if (!all_option.idct_fast) {
      calc_coef(stockage_coef);
   }

   uint8_t y_id, cb_id, cr_id, yhf, yvf, cbhf, cbvf, crhf, crvf;
   uint64_t nb_blocYH, nb_blocCbH, nb_blocCrH;
   char *rgb;
   if (nbcomp == 3) {
      for (uint8_t i = 0; i < nbcomp; i++) {
         if (img->comps->comps[0]->idc == img->comps->ordre[i]) y_id = i;
         if (img->comps->comps[1]->idc == img->comps->ordre[i]) cb_id = i;
         if (img->comps->comps[2]->idc == img->comps->ordre[i]) cr_id = i;
      }
      y_id = 0;
      cb_id = 1;
      cr_id = 2;
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

   uint64_t nb_passage_sos = 1;

   while (!img->section->eoi_done) {
      // Tableau contenant les dc précédant le bloc en cours de traitement (initialement 0 pour toutes les composantes)
      int16_t *dc_prec = (int16_t *)calloc(nbcomp, sizeof(int16_t));
      uint8_t off = 0;
      uint16_t *skip_blocs = (uint16_t *)calloc(nbcomp, sizeof(uint16_t));
      for (uint64_t i = 0; i < img->nbMCU; i++) {
         print_v("MCU %d, %d, %d, %d\n", i, img->nbmcuH, img->nbmcuV, img->nbMCU);
         uint64_t mcuX = i % img->nbmcuH;
         uint64_t mcuY = i / img->nbmcuH;
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
            uint8_t hs = img->comps->comps[indice_comp]->hsampling;
            uint8_t vs = img->comps->comps[indice_comp]->vsampling;
            print_v("COMP %d, %d, %d, %d, %d\n", indice_comp, hs, vs, img->max_hsampling, img->max_vsampling);
            print_v("INFO %d, %d, %d, %d\n", img->other->ss, img->other->se, img->other->ah, img->other->al);
            uint64_t nbH = img->nbmcuH * hs;
            for (uint8_t by = 0; by < vs; by++) {
               for (uint8_t bx = 0; bx < hs; bx++) {
                  if (skip_blocs[indice_comp] == 0) {
		     print_v("BLOC %d\n", by * hs + bx);
		     uint64_t blocX = mcuX * hs + bx;
		     uint64_t blocY = mcuY * vs + by;
		     uint16_t skip_bloc = decode_bloc_progressive(infile, img, indice_comp, sortieq[indice_comp][blocY * nbH + blocX], dc_prec, &off);
		     if (stop) break;
		     skip_blocs[indice_comp] = skip_bloc;
		  } else {
		     skip_blocs[indice_comp]--;
		  }
		  if (stop) break;
               }
	       if (stop) break;
            }
	    if (stop) break;
         }
	 if (stop) break;
      }
      // Si termine par ff 00 puis ff marker alors skip le 00 pour aller sur le 2e ff
      char dernier = fgetc(infile);
      /*if (dernier == (char) 0xff) {
	 char suivant_dernier = fgetc(infile);
	 if (suivant_dernier != (char) 0x00) {
	    erreur("Il faut un 0x00 après 0xff (%d)", ftell(infile));
	 }
	 }*/
      if (stop) {
	 printf("deb : %lx\n", ftell(infile));
	 while (fgetc(infile) != 0xff) {
	    fseek(infile, -2, SEEK_CUR);
	    printf("aa\n");
	 }
	 fseek(infile, -1, SEEK_CUR);
      }
      printf("%lx\n", ftell(infile));
      free(skip_blocs);
      stop = false;

      print_v("Fin données sos : %x\n", (int)ftell(infile));

      FILE *outputfile = ouverture_fichier_out(nbcomp, nb_passage_sos);

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

      for (uint64_t i = 0; i < img->nbMCU; i++) {
         // print_v("MCU %d\n", i);
         uint64_t mcuX = i % img->nbmcuH;
         uint64_t mcuY = i / img->nbmcuH;
         for (uint8_t k = 0; k < nbcomp; k++) {
            // print_v("COMP %d\n", k);
            uint64_t nbH = img->nbmcuH * img->comps->comps[k]->hsampling;
            for (uint8_t by = 0; by < img->comps->comps[k]->vsampling; by++) {
               for (uint8_t bx = 0; bx < img->comps->comps[k]->hsampling; bx++) {
                  // print_v("BLOC %d\n", by*img->comps->comps[k]->hsampling+bx);
                  uint64_t blocX = mcuX * img->comps->comps[k]->hsampling + bx;
                  uint64_t blocY = mcuY * img->comps->comps[k]->vsampling + by;
                  bloct16_t *bloc_zz = izz(sortieq[k][blocY * nbH + blocX]);
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
	    if (nbcomp == 1) {
	       save_mcu_ligne_bw(outputfile, img, ycc);
	    } else if (nbcomp == 3) {
	       save_mcu_ligne_color(outputfile, img, ycc, rgb, yhf, yvf, y_id, nb_blocYH, cbhf, cbvf, cb_id, nb_blocCbH, crhf, crvf, cr_id, nb_blocCrH);
	    }
         }
      }

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

      decode_entete(infile, false, img);
      nb_passage_sos++;
   }

   if (nbcomp == 3) free(rgb);

   // Free sortieq
   for (uint8_t i = 0; i < nbcomp; i++) {
      uint64_t nbH = img->nbmcuH * img->comps->comps[i]->hsampling;
      uint64_t nbV = img->nbmcuV * img->comps->comps[i]->vsampling;
      for (uint64_t j = 0; j < nbH * nbV; j++) {
         free(sortieq[i][j]);
      }
      free(sortieq[i]);
   }
   free(sortieq);
}
