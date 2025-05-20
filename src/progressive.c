#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <utils.h>
#include <options.h>
#include <idct.h>
#include <idct_opt.h>
#include <vld.h>
#include <iqzz.h>
#include <bitstream.h>
#include <decoder_utils.h>
#include <progressive.h>
#include <entete.h>
#include <timer.h>


extern all_option_t all_option;

// Place un bloc décodé en progressif après quantification inverse dans <*sortie>, lu à partir de <bs>.
// img     : contient les informations sur l'image, nécessaires au décodage et à la déquantification
// comp    : indice de la composante traitée
// dc_prec : tableau 1xnb_comp contenant les coefficients DC précédents pour chaque composante (nécessaire au décodage du suivant)
static erreur_t decode_bloc_progressive(bitstream_t *bs, img_t *img, int comp, blocl16_t *sortie, int16_t *dc_prec, uint16_t *skip_bloc);

// Initialise et retourne le tableau qui stockera les pixels de l'image décodée.
static blocl16_t ***init_sortie(img_t *img);

// Libère le tableau <*sortie> (initialisé par `init_sortie`).
static void free_sortie(img_t *img, blocl16_t ***sortie);

// Décode tous les coefficients DC, lus sur <*bs> et l'écrit dans <sortieq>.
static erreur_t decode_progressif_dc(bitstream_t *bs, img_t *img, blocl16_t ***sortieq);

// Décode tous les coefficients AC, lus sur <*bs> et l'écrit dans <sortieq>.
static erreur_t decode_progressif_ac(bitstream_t *bs, img_t *img, blocl16_t ***sortieq);

// Libère la structure <ycc> (allouée dans `decode_progressive_image`).
static void free_ycc(img_t *img, bloctu8_t ***ycc);


static erreur_t decode_bloc_progressive(bitstream_t *bs, img_t *img, int comp, blocl16_t *sortie, int16_t *dc_prec, uint16_t *skip_bloc) {
   uint8_t s_start = img->other->ss;
   uint8_t s_end = img->other->se;
   // On récupère les tables de Huffman et de quantification pour la composante courante
   huffman_tree_t *hdc = NULL;
   huffman_tree_t *hac = NULL;
   hdc = img->htables->dc[img->comps->comps[comp]->idhdc];
   hac = img->htables->ac[img->comps->comps[comp]->idhac];

   // S'il manque une table on retourne une erreur
   if (s_start == 0 && hdc == NULL) {
      char *str = malloc(80);
      sprintf(str, "Pas de table de huffman DC pour la composante %d", comp);
      return (erreur_t) {.code = ERR_NO_HT, .com = str, .must_free = true};
   }
   if (s_end != 0 && hac == NULL) {
      char *str = malloc(80);
      sprintf(str, "Pas de table de huffman AC pour la composante %d", comp);
      return (erreur_t) {.code = ERR_NO_HT, .com = str, .must_free = true};
   }

   // On décode un bloc de l'image (et on chronomètre le temps)
   int16_t *dc_prec_comp = (dc_prec != NULL) ? dc_prec + comp : NULL;
   erreur_t err = decode_bloc_acdc(bs, img, hdc, hac, sortie, dc_prec_comp, skip_bloc);
   if (err.code) return err;
   if (*skip_bloc != 0) (*skip_bloc)--;

   return (erreur_t) {.code = SUCCESS};
}

static blocl16_t ***init_sortie(img_t *img){
   blocl16_t ***sortie = (blocl16_t ***)malloc(sizeof(blocl16_t **) * img->comps->nb);
   for (uint8_t i = 0; i < img->comps->nb; i++) {
      uint64_t nbH = img->nbmcuH * img->comps->comps[i]->hsampling;
      uint64_t nbV = img->nbmcuV * img->comps->comps[i]->vsampling;
      sortie[i] = (blocl16_t **)malloc(nbH * nbV * sizeof(blocl16_t));
      for (uint64_t j = 0; j < nbH * nbV; j++) {
         sortie[i][j] = (blocl16_t *)calloc(1, sizeof(blocl16_t));
      }
   }
   return sortie;
}

static void free_sortie(img_t *img, blocl16_t ***sortie) {
   for (uint8_t i = 0; i < img->comps->nb; i++) {
      uint64_t nbH = img->nbmcuH * img->comps->comps[i]->hsampling;
      uint64_t nbV = img->nbmcuV * img->comps->comps[i]->vsampling;
      for (uint64_t j = 0; j < nbH * nbV; j++) {
         free(sortie[i][j]);
      }
      free(sortie[i]);
   }
   free(sortie);
}

static erreur_t decode_progressif_dc(bitstream_t *bs, img_t *img, blocl16_t ***sortieq) {
   const uint8_t nbcomp = img->comps->nb;
   // Tableau contenant les dc précédant le bloc en cours de traitement (initialement 0 pour toutes les composantes)
   int16_t *dc_prec = (int16_t *)calloc(nbcomp, sizeof(int16_t));
   for (uint64_t i = 0; i < img->nbMCU; i++) { // parcours des MCUs
      const uint64_t mcuX = i % img->nbmcuH;
      const uint64_t mcuY = i / img->nbmcuH;
      for (uint8_t k = 0; k < nbcomp; k++) {
         const int16_t indice_comp = get_composante(img, k);
         if (indice_comp == -1) break; // cas où la composante n'est pas renseignée dans <*img>
         const uint8_t hs = img->comps->comps[indice_comp]->hsampling;
         const uint8_t vs = img->comps->comps[indice_comp]->vsampling;
         const uint64_t nbH = img->nbmcuH * hs;
         // Parcours les blocs de la MCU
         for (uint8_t by = 0; by < vs; by++) {
            for (uint8_t bx = 0; bx < hs; bx++) {
               const uint64_t blocX = mcuX * hs + bx;
               const uint64_t blocY = mcuY * vs + by;
               uint16_t skip_bloc;
               // Décodage du bloc
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
   // cas où la composante n'est pas renseignée dans <*img>
   if (indice_comp == -1) return (erreur_t) {.code = ERR_COMP_ID, .com = "Aucune composante dans le scan", .must_free = false};

   const uint64_t nb_blocH = ceil((double)img->width / 8);
   const uint64_t nb_blocV = ceil((double)img->height / 8);

   // Nombre de blocs horizontalement et verticalement dans la MCU
   const uint8_t hs = img->comps->comps[indice_comp]->hsampling;
   const uint8_t vs = img->comps->comps[indice_comp]->vsampling;
   // facteur d'échantillonnage horizontal et vertical
   const uint8_t hf = img->max_hsampling / hs;
   const uint8_t vf = img->max_vsampling / vs;

   // nb_totalH : nombre de bloc horizontalement en complétant la dernière MCU
   const uint64_t nb_totalH = img->nbmcuH * hs;
   // nbH : nombre de bloc horizontalement sans compléter la dernière MCU
   // nbV : nombre de bloc verticalement sans compléter la dernière MCU
   const uint64_t nbH = ceil((double)nb_blocH / hf);
   const uint64_t nbV = ceil((double)nb_blocV / vf);

   // Parcours de tous les blocs de l'image
   for (uint64_t i = 0; i < nbH*nbV; i++) {
      uint64_t blocX = i % nbH;
      uint64_t blocY = i / nbH;
      // Si skip_blocs != 0 alors il y a eu un EOB et donc on corrige le bloc à la place de le décoder
      if (skip_blocs == 0) {
         erreur_t err = decode_bloc_progressive(bs, img, indice_comp, sortieq[indice_comp][blocY*nb_totalH + blocX], NULL, &skip_blocs);
         if (err.code) return err;
      } else {
         // On ne corrige que dans les approximations successives
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

static void free_ycc(img_t *img, bloctu8_t ***ycc) {
   // Free ycc
   for (uint8_t i = 0; i < img->comps->nb; i++) {
      uint64_t nbH = img->nbmcuH * img->comps->comps[i]->hsampling;
      uint64_t nbV = img->comps->comps[i]->vsampling;
      for (uint64_t j = 0; j < nbH * nbV; j++) {
         free(ycc[i][j]);
      }
      free(ycc[i]);
   }
   free(ycc);
}

erreur_t decode_progressive_image(FILE *infile, img_t *img) {
   const uint8_t nbcomp = img->comps->nb;
   blocl16_t ***sortie = init_sortie(img);
   // Calcul des coefficients pour la DCT inverse (lente)
   float stockage_coef[8][8][8][8];
   if (!all_option.idct_fast) {
      calc_coef(stockage_coef);
   }

   // Id, facteur horizontal, facteur vertical pour y, cb, et cr
   uint8_t y_id, cb_id, cr_id, yhf, yvf, cbhf, cbvf, crhf, crvf;
   // Nombre de bloc horizontalement pour y, cb, cr
   uint64_t nb_blocYH, nb_blocCbH, nb_blocCrH;
   // Tableau utilisé pour l'écriture dans le fichier ppm
   char *rgb;
   if (nbcomp == 3) get_ycc_info(img, &y_id, &cb_id, &cr_id, &yhf, &yvf, &cbhf, &cbvf, &crhf, &crvf, &nb_blocYH, &nb_blocCbH, &nb_blocCrH, &rgb);

   // Indice du scan en cours
   uint64_t nb_passage_sos = 1;

   my_timer_t timer_image;
   my_timer_t timer_ecriture;

   // Décodage de toute l'image
   while (!img->section->eoi_done) {
      // Initialisation du chronomètre
      init_timer(&timer_image);
      start_timer(&timer_image);
      init_timer(&timer_ecriture);

      bitstream_t *bs = (bitstream_t*) malloc(sizeof(bitstream_t));
      init_bitstream(bs, infile);

      // Décode le scan (DC ou AC)
      erreur_t err;
      if (img->other->se == 0) err = decode_progressif_dc(bs, img, sortie);
      else err = decode_progressif_ac(bs, img, sortie);
      if (err.code) {
         free(bs);
         free_sortie(img, sortie);
         return err;
      }

      // Terminer octet en cours avant de décoder des données d'entête
      err = finir_octet(bs);
      if (err.code) {
         free(bs);
         free_sortie(img, sortie);
         return err;
      }

      free(bs);

      // Écriture du scan
      FILE *outputfile = ouverture_fichier_out(nbcomp, nb_passage_sos);

      if (nbcomp == 1) fprintf(outputfile, "P5\n");
      else if (nbcomp == 3) fprintf(outputfile, "P6\n");
      fprintf(outputfile, "%d %d\n", img->width, img->height); // largeur, hauteur
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

      // On termine le décodage des MCU et on écrit l'image ppm dans un fichier de sortie
      for (uint64_t i = 0; i < img->nbMCU; i++) {
         uint64_t mcuX = i % img->nbmcuH;
         uint64_t mcuY = i / img->nbmcuH;
         for (uint8_t k = 0; k < nbcomp; k++) {
            qtable_prec_t *qtable = NULL;
            qtable = img->qtables[img->comps->comps[k]->idq];
            if (qtable == NULL) {
               char *str = malloc(80);
               sprintf(str, "Pas de table de quantification pour la composante %d", k);
               return (erreur_t) {.code = ERR_NO_HT, .com = str, .must_free = true};
            }

            // Nombre de bloc horizontalement (en complétant la MCU)
            uint64_t nbH = img->nbmcuH * img->comps->comps[k]->hsampling;
            // Parcours sur les blocs de la MCU
            for (uint8_t by = 0; by < img->comps->comps[k]->vsampling; by++) {
               for (uint8_t bx = 0; bx < img->comps->comps[k]->hsampling; bx++) {
                  uint64_t blocX = mcuX * img->comps->comps[k]->hsampling + bx;
                  uint64_t blocY = mcuY * img->comps->comps[k]->vsampling + by;
                  // quantification inverse
                  blocl16_t *quant = (blocl16_t*) calloc(1, sizeof(blocl16_t));
                  for (int i=0; i<64; i++) quant->data[i] = sortie[k][blocY * nbH + blocX]->data[i];
                  iquant(quant, 0, 63, qtable->qtable);
                  // zig-zag inverse
                  bloct16_t *bloc_zz = izz(quant);
                  free(quant);
                  // DCT inverse
                  bloctu8_t *bloc_idct;
                  if (all_option.idct_fast) bloc_idct = idct_opt(bloc_zz);
                  else bloc_idct = idct(bloc_zz, stockage_coef);
                  free(bloc_zz);
                  // Stockage dans ycc
                  if (ycc[k][by * nbH + blocX] != NULL) free(ycc[k][by * nbH + blocX]);
                  ycc[k][by * nbH + blocX] = bloc_idct;
               }
            }
         }
         // Si une ligne de MCU, alors on écrit dans le fichier ppm
         if (i % img->nbmcuH == img->nbmcuH - 1) {
            stop_timer(&timer_image);
            start_timer(&timer_ecriture);
            erreur_t err;
            if (nbcomp == 1) {
               err = save_mcu_ligne_bw(outputfile, img, ycc);
            } else if (nbcomp == 3) {
               err = save_mcu_ligne_color(outputfile, img, ycc, rgb, yhf, yvf, y_id, nb_blocYH, cbhf, cbvf, cb_id, nb_blocCbH, crhf, crvf, cr_id, nb_blocCrH);
            }
            if (err.code) {
               free_sortie(img, sortie);
               free_ycc(img, ycc);
               if (nbcomp == 3) free(rgb);
            }
            stop_timer(&timer_ecriture);
            start_timer(&timer_image);
         }
      }

      fclose(outputfile);
      free_ycc(img, ycc);

      char str[43];
      sprintf(str, "Décodage de l'image n°%ld", nb_passage_sos);
      print_timer(str, &timer_image);
      sprintf(str, "Ecriture de l'image n°%ld", nb_passage_sos);
      print_timer(str, &timer_ecriture);

      // On décode les entêtes avant le scan suivant
      err = decode_entete(infile, false, img);
      if (err.code) {
         free_sortie(img, sortie);
         if (nbcomp == 3) free(rgb);
         return err;
      }
      nb_passage_sos++;
   }

   // Création d'un lien symbolique pour la dernière image décodée
   char *base_out_name = out_file_name(nbcomp, 0);
   char *out_name = out_file_name(nbcomp, nb_passage_sos-1);
   char *sl = strrchr(out_name, '/');
   char *cmd = malloc(sizeof(char)*(11+strlen(sl+1)+strlen(base_out_name)));
   sprintf(cmd, "ln -f -s %s %s", sl+1, base_out_name);
   (void) !system(cmd);
   free(out_name);
   free(base_out_name);
   free(cmd);

   if (nbcomp == 3) free(rgb);

   free_sortie(img, sortie);

   return (erreur_t) {.code = SUCCESS};
}
