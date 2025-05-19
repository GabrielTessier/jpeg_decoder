#include <stdio.h>
#include <stdlib.h>
#include <ycc2rgb.h>
#include <utils.h>
#include <options.h>
#include <decoder_utils.h>

extern all_option_t all_option;

void print_huffman_quant_table(img_t *img) {
   if (all_option.print_tables) {
      for (uint8_t i = 0; i < 4; i++) {
         // Affichage tables de Huffman
         if (img->htables->dc[i] != NULL) {
            printf("Huffman dc %d\n", i);
            print_hufftable(img->htables->dc[i]);
         }
         if (img->htables->ac[i] != NULL) {
            printf("Huffman ac %d\n", i);
            print_hufftable(img->htables->ac[i]);
         }
      }
      for (uint8_t i = 0; i < 4; i++) {
         // Affichage tables de quantification
         if (img->qtables[i] != NULL) {
            printf("Table de quantification %d : ", i);
            for (uint8_t j = 0; j < 64; j++) {
               printf("%d, ", img->qtables[i]->qtable->data[j]);
            }
            printf("\n");
         }
      }
   }
}

int16_t get_composante(img_t *img, uint8_t k) {
   const uint8_t nbcomp = img->comps->nb;
   uint8_t idcomp = img->comps->ordre[k];
   if (idcomp == 0) return -1;
   for (uint8_t c = 0; c < nbcomp; c++) {
      if (img->comps->comps[c]->idc == idcomp) {
         return c;
      }
   }
   return -1;
}

erreur_t save_mcu_ligne_bw(FILE *outputfile, img_t *img, bloctu8_t ***ycc) {
   const uint8_t nbcomp = img->comps->nb;
   if (nbcomp != 1) return (erreur_t) {.code = ERR_NB_COMP, .com = "Il faut 1 composante pour save_mcu_ligne_bw", .must_free = false};
   for (uint64_t y = 0; y < img->max_vsampling * 8; y++) {
      for (uint64_t x = 0; x < img->width; x++) {
         if (nbcomp == 1) {
            // On print le pixel de coordonnée (x,y)
            uint64_t bx = x / 8; // bx-ieme bloc horizontalement
            uint64_t px = x % 8;
            uint64_t py = y % 8; // le pixel est à la coordonnée (px,py) du blob (bx,by)
            fwrite(&(ycc[0][bx]->data[px][py]), sizeof(char), 1, outputfile);
         }
      }
   }
   return (erreur_t) {.code = SUCCESS};
}

erreur_t save_mcu_ligne_color(FILE *outputfile, img_t *img, bloctu8_t ***ycc, char *rgb, uint8_t yhf, uint8_t yvf, uint8_t y_id, uint64_t nb_blocYH, uint8_t cbhf, uint8_t cbvf, uint8_t cb_id, uint64_t nb_blocCbH, uint8_t crhf, uint8_t crvf, uint8_t cr_id, uint64_t nb_blocCrH) {
   const uint8_t nbcomp = img->comps->nb;
   if (nbcomp != 3) return (erreur_t) {.code = ERR_NB_COMP, .com = "Il faut 3 composantes pour save_mcu_ligne_color", .must_free = false};
   for (uint64_t y = 0; y < img->max_vsampling * 8; y++) {
      for (uint64_t x = 0; x < img->width; x++) {
         // On print le pixel de coordonnée (x,y)
         uint64_t px, py;
         px = x / yhf;
         py = y / yvf;
         int8_t y_ycc = ycc[y_id][(py >> 3) * nb_blocYH + (px >> 3)]->data[px % 8][py % 8];
         px = x / cbhf;
         py = y / cbvf;
         int8_t cb_ycc = ycc[cb_id][(py >> 3) * nb_blocCbH + (px >> 3)]->data[px % 8][py % 8];
         px = x / crhf;
         py = y / crvf;
         int8_t cr_ycc = ycc[cr_id][(py >> 3) * nb_blocCrH + (px >> 3)]->data[px % 8][py % 8];
         rgb_t pixel_rgb;
         ycc2rgb_pixel(y_ycc, cb_ycc, cr_ycc, &pixel_rgb);
         rgb[x * 3 + 0] = pixel_rgb.r;
         rgb[x * 3 + 1] = pixel_rgb.g;
         rgb[x * 3 + 2] = pixel_rgb.b;
      }
      fwrite(rgb, sizeof(char), img->width * 3, outputfile);
   }
   return (erreur_t) {.code = SUCCESS};
}

void get_ycc_info(img_t *img, uint8_t *y_id, uint8_t *cb_id, uint8_t *cr_id, uint8_t *yhf, uint8_t *yvf, uint8_t *cbhf, uint8_t *cbvf, uint8_t *crhf, uint8_t *crvf, uint64_t *nb_blocYH, uint64_t *nb_blocCbH, uint64_t *nb_blocCrH, char **rgb) {
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
