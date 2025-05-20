#include <jpeg2ppm.h>
#include <stdio.h>
#include <stdlib.h>

#include <utils.h>
#include <timer.h>
#include <idct.h>
#include <idct_opt.h>
#include <options.h>
#include <bitstream.h>
#include <decoder_utils.h>
#include <iqzz.h>
#include <vld.h>
#include <baseline.h>


extern all_option_t all_option;

// Place un bloc décodé en baseline après quantification inverse dans <*sortie>,
// lu à partir de <bs>.
// img     : contient les informations sur l'image, nécessaires au décodage et à la déquantification.
// comp    : indice de la composante traitée
// dc_prec : tableau 1xnb_comp contenant les coefficients DC précédents pour chaque composante (nécessaire au décodage du suivant)
static erreur_t decode_bloc_baseline(bitstream_t *bs, img_t *img, int comp, blocl16_t *sortie, int16_t *dc_prec);

// Libère la mémoire allouée par decode_baseline_image.
// Les paramètres sont les différents objets à libérer.
static void baseline_free(img_t *img, FILE *outputfile, bloctu8_t ***ycc, int16_t *dc_prec, char *rgb, bitstream_t *bs);


static erreur_t decode_bloc_baseline(bitstream_t *bs, img_t *img, int comp, blocl16_t *sortie, int16_t *dc_prec) {
   // On récupère les tables de Huffman et de quantification pour la composante courante
   huffman_tree_t *hdc = NULL;
   huffman_tree_t *hac = NULL;
   qtable_prec_t *qtable = NULL;
   hdc = img->htables->dc[img->comps->comps[comp]->idhdc];
   hac = img->htables->ac[img->comps->comps[comp]->idhac];
   qtable = img->qtables[img->comps->comps[comp]->idq];

   // S'il manque une table on retourne une erreur
   if (hdc == NULL) {
      char *str = malloc(80);
      sprintf(str, "Pas de table de huffman DC pour la composante %d", comp);
      return (erreur_t) {.code = ERR_NO_HT, .com = str, .must_free = true};
   }
   if (hac == NULL) {
      char *str = malloc(80);
      sprintf(str, "Pas de table de huffman AC pour la composante %d", comp);
      return (erreur_t) {.code = ERR_NO_HT, .com = str, .must_free = true};
   }
   if (qtable == NULL) {
      char *str = malloc(80);
      sprintf(str, "Pas de table de quantification pour la composante %d", comp);
      return (erreur_t) {.code = ERR_NO_HT, .com = str, .must_free = true};
   }

   // On décode un bloc de l'image
   uint16_t skip_bloc;
   erreur_t err = decode_bloc_acdc(bs, img, hdc, hac, sortie, dc_prec + comp, &skip_bloc);
   if (err.code) return err;
   if (skip_bloc > 1) {
      return (erreur_t) {.code = ERR_AC_BAD, .com = "Symbole RLE interdit en baseline", .must_free = false};
   }
   
   // On fait la quantification inverse (en place)
   iquant(sortie, img->other->ss, img->other->se, qtable->qtable);
   
   return (erreur_t) {.code = SUCCESS};
}

static void baseline_free(img_t *img, FILE *outputfile, bloctu8_t ***ycc, int16_t *dc_prec, char *rgb, bitstream_t *bs) {
   const uint8_t nbcomp = img->comps->nb;
   free(bs);
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

erreur_t decode_baseline_image(FILE *infile, img_t *img) {
   // Affiche les tables de huffman et de quantification (si option print_tables)
   if (all_option.print_tables) print_huffman_quant_table(img);
   
   const uint8_t nbcomp = img->comps->nb;
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

   // Id, facteur horizontal, facteur vertical pour y, cb, et cr
   uint8_t y_id, cb_id, cr_id, yhf, yvf, cbhf, cbvf, crhf, crvf;
   // Nombre de bloc horizontalement pour y, cb, cr
   uint64_t nb_blocYH, nb_blocCbH, nb_blocCrH;
   // Tableau utilisé pour l'écriture dans le fichier ppm
   char *rgb;
   if (nbcomp == 3) get_ycc_info(img, &y_id, &cb_id, &cr_id, &yhf, &yvf, &cbhf, &cbvf, &crhf, &crvf, &nb_blocYH, &nb_blocCbH, &nb_blocCrH, &rgb);

   FILE *outputfile = ouverture_fichier_out(nbcomp, 0);

   // P5 : noir et blanc
   // P6 : couleur RGB
   if (nbcomp == 1) {
      fprintf(outputfile, "P5\n");
   } else if (nbcomp == 3) {
      fprintf(outputfile, "P6\n");
   } else {
      fclose(outputfile);
      return (erreur_t) {.code = ERR_NB_COMP, .com = "Il faut une ou trois composante", .must_free = false};
   }
   fprintf(outputfile, "%d %d\n", img->width, img->height); // largeur, hauteur
   fprintf(outputfile, "255\n");

   // Initialisation du tableau ycc :
   // ycc contient un tableau par composante
   // Chaque sous-tableau est de taille le nombre de bloc pour une ligne de mcu dans la composante (et stocke des blocs 8x8)
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
   my_timer_t timer_ecriture;
   init_timer(&timer_ecriture);
   

   // Tableau contenant les DC précédant le bloc en cours de traitement (initialement 0 pour toutes les composantes)
   int16_t *dc_prec = (int16_t *)calloc(nbcomp, sizeof(int16_t));

   bitstream_t *bs = (bitstream_t*) malloc(sizeof(bitstream_t));
   init_bitstream(bs, infile);

   // Pour chaque MCU, on la décode
   for (uint64_t i = 0; i < img->nbMCU; i++) {
      uint64_t mcuX = i % img->nbmcuH;
      for (uint8_t k = 0; k < nbcomp; k++) {
         int16_t indice_comp = get_composante(img, k);
         if (indice_comp == -1) break;   // Si un scan ne contient pas toutes les composantes
         uint64_t nbH = img->nbmcuH * img->comps->comps[indice_comp]->hsampling;
         // On itère sur chaque bloc de la MCU
         for (uint8_t by = 0; by < img->comps->comps[indice_comp]->vsampling; by++) {
            for (uint8_t bx = 0; bx < img->comps->comps[indice_comp]->hsampling; bx++) {
               uint64_t blocX = mcuX * img->comps->comps[indice_comp]->hsampling + bx;
               blocl16_t *bloc = (blocl16_t*) calloc(1, sizeof(blocl16_t));
               
               // Décodage d'un bloc (baseline) et déquantification
               start_timer(&timer_decode_bloc);
               erreur_t err = decode_bloc_baseline(bs, img, indice_comp, bloc, dc_prec);
               stop_timer(&timer_decode_bloc);
               if (err.code) {
                  baseline_free(img, outputfile, ycc, dc_prec, rgb, bs);
                  return err;
               }

	       // Inversion de la transformation zig-zag
               start_timer(&timer_izz);
               bloct16_t *bloc_zz = izz(bloc);
               stop_timer(&timer_izz);
               free(bloc);

	       // IDCT
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
      // Écrit une ligne de MCUs dans le fichier de destination
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
            baseline_free(img, outputfile, ycc, dc_prec, rgb, bs);
            return err;
         }
         stop_timer(&timer_ecriture);
         start_timer(&timer_image);
      }
   }

   erreur_t err = finir_octet(bs);
   baseline_free(img, outputfile, ycc, dc_prec, rgb, bs);
   if (err.code) return err;

   // Affichage des chronomètres dans la sortie standard conditionné aux options en ligne de commande
   print_timer("Décodage DC/AC et Quantification", &timer_decode_bloc);
   print_timer("IZZ", &timer_izz);
   print_timer("IDCT", &timer_idct);
   print_timer("Décodage complet de l'image", &timer_image);
   print_timer("Ecriture de l'image", &timer_ecriture);

   return (erreur_t) {.code = SUCCESS};
}
