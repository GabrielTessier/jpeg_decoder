
#include <math.h>
#include <stdio.h>

#include <utils.h>
#include <options.h>
#include <idct.h>
#include <idct_opt.h>
#include <vld.h>
#include <iqzz.h>
#include <bitstream.h>
#include <decoder_utils.h>
#include <progressive.h>

extern all_option_t all_option;

static erreur_t decode_bloc_progressive(bitstream_t *bs, img_t *img, int comp, blocl16_t *sortie, int16_t *dc_prec, uint16_t *skip_bloc) {
   uint8_t s_start = img->other->ss;
   uint8_t s_end = img->other->se;
   // On récupère les tables de Huffman et de quantification pour la composante courante
   huffman_tree_t *hdc = NULL;
   huffman_tree_t *hac = NULL;
   hdc = img->htables->dc[img->comps->comps[comp]->idhdc];
   hac = img->htables->ac[img->comps->comps[comp]->idhac];

   // S'il manque une table on exit avec une erreur
   if (s_start == 0 && hdc == NULL) {
      char *str = malloc(80);
      sprintf(str, "Pas de table de huffman DC pour la composante %d", comp);
      return (erreur_t) {.code = ERR_NO_HT, .com = str};
   }
   if (s_end != 0 && hac == NULL) {
      char *str = malloc(80);
      sprintf(str, "Pas de table de huffman AC pour la composante %d", comp);
      return (erreur_t) {.code = ERR_NO_HT, .com = str};
   }

   // On décode un bloc de l'image (et on chronomètre le temps)
   int16_t *dc_prec_comp = (dc_prec != NULL) ? dc_prec + comp : NULL;
   erreur_t err = decode_bloc_acdc(bs, img, hdc, hac, sortie, dc_prec_comp, skip_bloc);
   if (err.code) return err;
   if (*skip_bloc != 0) (*skip_bloc)--;

   return (erreur_t) {.code = SUCCESS};
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

static erreur_t decode_progressif_dc(bitstream_t *bs, img_t *img, blocl16_t ***sortieq) {
   const uint8_t nbcomp = img->comps->nb;
   int16_t *dc_prec = (int16_t *)calloc(nbcomp, sizeof(int16_t));
   for (uint64_t i = 0; i < img->nbMCU; i++) {
      print_v("MCU %d, %d, %d, %d\n", i, img->nbmcuH, img->nbmcuV, img->nbMCU);
      uint64_t mcuX = i % img->nbmcuH;
      uint64_t mcuY = i / img->nbmcuH;
      for (uint8_t k = 0; k < nbcomp; k++) {
	 int16_t indice_comp = get_composante(img, k);
	 if (indice_comp == -1) break;
	    
	 uint8_t hs = img->comps->comps[indice_comp]->hsampling;
	 uint8_t vs = img->comps->comps[indice_comp]->vsampling;
	 print_v("COMP %d, %d, %d, %d, %d\n", indice_comp, hs, vs, img->max_hsampling, img->max_vsampling);
	 print_v("INFO %d, %d, %d, %d\n", img->other->ss, img->other->se, img->other->ah, img->other->al);
	 uint64_t nbH = img->nbmcuH * hs;
	 for (uint8_t by = 0; by < vs; by++) {
	    for (uint8_t bx = 0; bx < hs; bx++) {
	       print_v("BLOC %d\n", by * hs + bx);
	       uint64_t blocX = mcuX * hs + bx;
	       uint64_t blocY = mcuY * vs + by;
	       uint16_t skip_bloc;
	       erreur_t err = decode_bloc_progressive(bs, img, indice_comp, sortieq[indice_comp][blocY * nbH + blocX], dc_prec, &skip_bloc);
	       if (err.code) return err;
	    }
	 }
      }
   }
   free(dc_prec);
   return (erreur_t) {.code = SUCCESS};
}

static erreur_t decode_progressif_ac(bitstream_t *bs, img_t *img, blocl16_t ***sortieq) {
   uint16_t skip_blocs = 0;
   int16_t indice_comp = get_composante(img, 0);
   if (indice_comp == -1) return (erreur_t) {.code = ERR_COMP_ID, "Aucune composante dans le scan"};

   uint64_t nb_blocH = ceil((double)img->width / 8);
   uint64_t nb_blocV = ceil((double)img->height / 8);
   
   uint8_t hs = img->comps->comps[indice_comp]->hsampling;
   uint8_t vs = img->comps->comps[indice_comp]->vsampling;
   uint8_t hf = img->max_hsampling / hs;
   uint8_t vf = img->max_vsampling / vs;
   
   uint64_t nb_totalH = img->nbmcuH * hs;
   uint64_t nbH = ceil((double)nb_blocH / hf);
   uint64_t nbV = ceil((double)nb_blocV / vf);
   
   for (uint64_t i = 0; i < nbH*nbV; i++) {
      print_v("BLOC %d, %d, %d, %d\n", i, nbH*nbV, img->nbmcuH, img->nbmcuV);
      print_v("COMP %d, %d, %d, %d, %d\n", indice_comp, hs, vs, img->max_hsampling, img->max_vsampling);
      print_v("INFO %d, %d, %d, %d\n", img->other->ss, img->other->se, img->other->ah, img->other->al);
      uint64_t blocX = i % nbH;
      uint64_t blocY = i / nbH;
      if (skip_blocs == 0) {
	 erreur_t err = decode_bloc_progressive(bs, img, indice_comp, sortieq[indice_comp][blocY*nb_totalH + blocX], NULL, &skip_blocs);
	 if (err.code) return err;
      } else {
	 if (img->other->ah != 0) {
	    uint64_t resi = img->other->ss;
	    erreur_t err = correction_eob(bs, img, sortieq[indice_comp][blocY*nb_totalH + blocX], &resi);
	    if (err.code) return err;
	 }
	 skip_blocs--;
      }
   }
   return (erreur_t) {.code = SUCCESS};
}

erreur_t decode_progressive_image(FILE *infile, img_t *img) {
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
      bitstream_t *bs = (bitstream_t*) malloc(sizeof(bitstream_t));
      init_bitstream(bs, infile);
      // Tableau contenant les dc précédant le bloc en cours de traitement (initialement 0 pour toutes les composantes)
      erreur_t err;
      if (img->other->se == 0) err = decode_progressif_dc(bs, img, sortieq);
      else err = decode_progressif_ac(bs, img, sortieq);
      if (err.code) return err;

      err = finir_octet(bs);
      if (err.code) return err;

      free(bs);

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
	    qtable_prec_t *qtable = NULL;
	    qtable = img->qtables[img->comps->comps[k]->idq];
	    if (qtable == NULL) {
	       char *str = malloc(80);
	       sprintf(str, "Pas de table de quantification pour la composante %d", k);
	       return (erreur_t) {.code = ERR_NO_HT, .com = str};
	    }
	    
            uint64_t nbH = img->nbmcuH * img->comps->comps[k]->hsampling;
            for (uint8_t by = 0; by < img->comps->comps[k]->vsampling; by++) {
               for (uint8_t bx = 0; bx < img->comps->comps[k]->hsampling; bx++) {
                  // print_v("BLOC %d\n", by*img->comps->comps[k]->hsampling+bx);
                  uint64_t blocX = mcuX * img->comps->comps[k]->hsampling + bx;
                  uint64_t blocY = mcuY * img->comps->comps[k]->vsampling + by;
		  blocl16_t *quant = (blocl16_t*) calloc(1, sizeof(blocl16_t));
		  for (int i=0; i<64; i++) quant->data[i] = sortieq[k][blocY * nbH + blocX]->data[i];
		  iquant(quant, 0, 63, qtable->qtable);
                  bloct16_t *bloc_zz = izz(quant);
		  free(quant);
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

      err = decode_entete(infile, false, img);
      if (err.code) return err;
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

   return (erreur_t) {.code = SUCCESS};
}
