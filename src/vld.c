#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include <vld.h>
#include <img.h>
#include <erreur.h>
#include <bitstream.h>


// Retourne la valeur correspondant à <indice> sachant la magnitude <magnitude>.
static int16_t get_val_from_magnitude(uint16_t magnitude, uint16_t indice);

// Place la valeur obtenue en lisant <magnitude> bits de <bs> dans <*val>.
static erreur_t read_val_from_magnitude(bitstream_t *bs, uint8_t magnitude, int16_t *val);
// Lit <nb_bit> bits et place l'entier lut dans <*indice>
static erreur_t read_indice(bitstream_t *bs, uint8_t nb_bit, uint16_t *indice);

// Décode un coefficient (DC) dans le premier scan (de la bande spectrale en cours) et le place dans <*coef_dc>
// symb_decode : une feuille de l'arbre de huffman (correspond à la magnitude)
static erreur_t decode_coef_DC_first_scan(bitstream_t *bs, img_t *img, huffman_tree_t *symb_decode, int16_t *coef_dc);

// Décode un coefficient (DC) dans un scan ultérieur au premier et le place dans <*coef_dc>
static erreur_t decode_coef_DC_subsequent_scan(bitstream_t *bs, img_t *img, int16_t *coef_dc);

// Décode un coefficient (AC) dans le premier scan (de la bande spectrale en cours) et le place dans <sortie[<*sortie_id>]>
// symb_decode : une feuille de l'arbre de huffman (correspond à la magnitude)
// skip_bloc : pointeur vers la variable qui contient le nombre de blocs à skip
static erreur_t decode_coef_AC_first_scan(bitstream_t *bs, img_t *img, huffman_tree_t *symb_decode, blocl16_t *sortie, uint64_t *sortie_id, uint16_t *skip_bloc);

// Corrige le coefficient <*coef>
static erreur_t correction_coef(bitstream_t *bs, img_t *img, int16_t *coef);

// Skip <n> coefficients nuls et corrige les coefficients non nuls sur le chemin
// <*indice_coef> est l'indice du coefficient en cours de traitement
static erreur_t correction_n_coef(bitstream_t *bs, img_t *img, uint16_t n, int16_t *coefs, uint64_t *indice_coef);

// Skip <n> coefficients nuls et corrige les coefficients non nuls sur le chemin puis corrige les coefficients non nuls jusqu'au prochain 0
// <*indice_coef> est l'indice du coefficient en cours de traitement
static erreur_t correction_n_coef_jusqua_zero(bitstream_t *bs, img_t *img, uint16_t n, int16_t *coefs, uint64_t *indice_coef);

// Assigne une valeur à un coefficient nul en suivant les règles a et b de la norme annexe G.1.2.3
static erreur_t skip_n_coef_AC_subsequent_scan(bitstream_t *bs, img_t *img, uint8_t n, blocl16_t *sortie, uint64_t *sortie_id);

// Skip 16 coefficients nul en corrigeant les coefficients non nuls
static erreur_t skip_16_coef_AC_subsequent_scan(bitstream_t *bs, img_t *img, blocl16_t *sortie, uint64_t *sortie_id);

// Décode un coefficient (AC) dans un scan ultérieur au premier (de la bande spectrale en cours) et le place dans <sortie[<*sortie_id>]>
// symb_decode : une feuille de l'arbre de huffman (correspond à la magnitude)
// skip_bloc : pointeur vers la variable qui contient le nombre de blocs à skip et corriger
static erreur_t decode_coef_AC_subsequent_scan(bitstream_t *bs, img_t *img, huffman_tree_t *symb_decode, blocl16_t *sortie, uint64_t *sortie_id, uint16_t *skip_bloc);

// Lit des bits jusqu'à atteindre une feuille dans l'arbre de huffman <ht>
static erreur_t get_huffman_symbole(bitstream_t *bs, huffman_tree_t **ht, bool *code_que_un);

// Décode un coefficient DC et le place dans sortie->data[0]
static erreur_t decode_coef_DC(bitstream_t *bs, img_t *img, huffman_tree_t* ht, blocl16_t *sortie);

// décode les coefficients d'une bande spectrale de AC et les place dans <sortie>
static erreur_t decode_list_coef_AC(bitstream_t *bs, img_t *img, huffman_tree_t* ht, blocl16_t *sortie, uint16_t *skip_bloc);

// Décode un bloc de DC/AC en baseline
// dc_prec : pointeur vers la valeur du DC précédant
static erreur_t decode_bloc_acdc_baseline(bitstream_t *bs, img_t *img, huffman_tree_t *hdc, huffman_tree_t *hac, blocl16_t *sortie, int16_t *dc_prec, uint16_t *skip_bloc);

// Décode un bloc de DC/AC en progressif
// dc_prec : pointeur vers la valeur du DC précédant
static erreur_t decode_bloc_acdc_progressif(bitstream_t *bs, img_t *img, huffman_tree_t *hdc, huffman_tree_t *hac, blocl16_t *sortie, int16_t *dc_prec, uint16_t *skip_bloc);




static int16_t get_val_from_magnitude(uint16_t magnitude, uint16_t indice) {
   if (magnitude == 0) return 0;
   // EX : pour magnitue de 3 :
   //      -7, -6, -5, -4, 4, 5, 6, 7
   //      min = 4
   //      max = 7
   int16_t min = 1<<(magnitude-1);
   int16_t max = (min<<1)-1;
   if (indice < min) return indice-max;  // Négatif
   return indice;
}

static erreur_t read_indice(bitstream_t *bs, uint8_t nb_bit, uint16_t *indice) {
   *indice = 0;
   // Pour chaque bit on l'ajoute par la gauche à indice
   while (nb_bit > 0) {
      uint8_t bit;
      erreur_t err = read_bit(bs, &bit);
      if (err.code) return err;
      *indice = (*indice) << 1;
      *indice += bit;
      nb_bit--;
   }
   return (erreur_t) {.code = SUCCESS};
}

static erreur_t read_val_from_magnitude(bitstream_t *bs, uint8_t magnitude, int16_t *val) {
   uint16_t indice;
   erreur_t err = read_indice(bs, magnitude, &indice);
   if (err.code) return err;
   *val = get_val_from_magnitude(magnitude, indice);
   return (erreur_t) {.code = SUCCESS};
}

static erreur_t decode_coef_DC_first_scan(bitstream_t *bs, img_t *img, huffman_tree_t *symb_decode, int16_t *coef_dc) {
   int16_t val;
   if (symb_decode->symb > 11) return (erreur_t) {.code = ERR_DC_BAD, .com = "La magnitude doit être inférieur ou égale à 11"};
   erreur_t err = read_val_from_magnitude(bs, symb_decode->symb, &val);
   if (err.code) return err;
   // Dans le cas baseline al = 0 donc le coef est val
   // Dans le cas progressif il faut multiplier par 2^al
   *coef_dc = val*(1<<img->other->al);
   return (erreur_t) {.code = SUCCESS};
}

static erreur_t decode_coef_DC_subsequent_scan(bitstream_t *bs, img_t *img, int16_t *coef_dc) {
   uint8_t bit;
   erreur_t err = read_bit(bs, &bit);
   if (err.code) return err;
   // Le bit lut est le al-ieme bit de coef_dc
   *coef_dc |= ((int16_t)bit)<<img->other->al;
   return (erreur_t) {.code = SUCCESS};
}

static erreur_t decode_coef_AC_first_scan(bitstream_t *bs, img_t *img, huffman_tree_t *symb_decode, blocl16_t *sortie, uint64_t *sortie_id, uint16_t *skip_bloc) {
   const uint8_t num_sof = img->section->num_sof;
   const uint8_t al = img->other->al;
   if (symb_decode->symb == 0xf0) {  // ZRL (on saute 16 coef)
      *sortie_id += 16;
   } else {
      const uint8_t alpha = symb_decode->symb >> 4;
      const uint8_t gamma = symb_decode->symb & 0b00001111;
      if (gamma == 0) {
         if (alpha == 0) {   // 0x00 EOB_0
            *skip_bloc = 1;
            return (erreur_t) {.code=SUCCESS};
         } else {
            // EOB_alpha (valide seulement en mode progressif)
            if (num_sof == 0) {  // baseline
               char *str = malloc(80);
               sprintf(str, "Code invalide pour AC (%x) car mode baseline", symb_decode->symb);
               return (erreur_t) {.code=ERR_AC_BAD, .com=str, .must_free = true};
            } else if (num_sof == 2) {  // progressif
               uint16_t indice;
	       // EOB_14 est le maximum
	       if (alpha > 14) {
		  char *str = (char*) malloc(sizeof(char)*27);
		  sprintf(str, "EOB%d interdit (max = 14)", alpha);
		  return (erreur_t) {.code = ERR_AC_BAD, .com = str, .must_free = true};
	       }
               erreur_t err = read_indice(bs, alpha, &indice);
               if (err.code) return err;
               *skip_bloc = indice + (1<<alpha);
               return (erreur_t) {.code=SUCCESS};
            } else {
               char *str = malloc(27);
               sprintf(str, "Numéro sof invalide : %d", num_sof);
               return (erreur_t) {.code=ERR_SOF_BAD, .com=str, .must_free = true};
            }
         }
      } else {  // 0xalpha gamma  (alpha coef nul puis coef de magnitude gamma)
         *sortie_id += alpha;
	 if (gamma > 10) return (erreur_t) {.code = ERR_AC_BAD, .com = "La magnitude doit être inférieur ou égale à 10"};
         erreur_t err = read_val_from_magnitude(bs, gamma, sortie->data + (*sortie_id));
         if (err.code) return err;
         sortie->data[*sortie_id] = sortie->data[*sortie_id]*(1<<al);
         (*sortie_id)++;
      }
   }
   *skip_bloc = 0;
   return (erreur_t) {.code=SUCCESS};
}

static erreur_t correction_coef(bitstream_t *bs, img_t *img, int16_t *coef) {
   uint8_t bit;
   erreur_t err = read_bit(bs, &bit);
   if (err.code) return err;
   if (bit == 1) *coef |= (int16_t)1<<img->other->al;
   return (erreur_t) {.code = SUCCESS};
}

static erreur_t correction_n_coef(bitstream_t *bs, img_t *img, uint16_t n, int16_t *coefs, uint64_t *indice_coef) {
   int i=0;
   while (i<n) {
      if (coefs[*indice_coef] != 0) {
         erreur_t err = correction_coef(bs, img, coefs + (*indice_coef));
         if (err.code) return err;
      } else {
         i++;
      }
      (*indice_coef)++;
   }
   return (erreur_t) {.code = SUCCESS};
}

static erreur_t correction_n_coef_jusqua_zero(bitstream_t *bs, img_t *img, uint16_t n, int16_t *coefs, uint64_t *indice_coef) {
   erreur_t err = correction_n_coef(bs, img, n, coefs, indice_coef);
   if (err.code) return err;
   // On lit jusqu'au prochain coefficient nul
   while (coefs[*indice_coef] != 0) {
      erreur_t err = correction_coef(bs, img, &(coefs[*indice_coef]));
      if (err.code) return err;
      (*indice_coef)++;
   }
   return (erreur_t) {.code = SUCCESS};
}

static erreur_t skip_n_coef_AC_subsequent_scan(bitstream_t *bs, img_t *img, uint8_t n, blocl16_t *sortie, uint64_t *sortie_id) {
   int16_t val = 0;
   erreur_t err = read_val_from_magnitude(bs, 1, &val);
   if (err.code) return err;

   err = correction_n_coef_jusqua_zero(bs, img, n, sortie->data, sortie_id);
   if (err.code) return err;
   
   sortie->data[*sortie_id] = val*(1<<img->other->al);
   (*sortie_id)++;
   return (erreur_t) {.code = SUCCESS};
}

static erreur_t skip_16_coef_AC_subsequent_scan(bitstream_t *bs, img_t *img, blocl16_t *sortie, uint64_t *sortie_id) {
   return correction_n_coef(bs, img, 16, sortie->data, sortie_id);
}

erreur_t correction_eob(bitstream_t *bs, img_t *img, blocl16_t *sortie, uint64_t *sortie_id) {
   while (*sortie_id <= img->other->se) {
      if (sortie->data[*sortie_id] != 0) {
         erreur_t err = correction_coef(bs, img, &(sortie->data[*sortie_id]));
         if (err.code) return err;
      }
      (*sortie_id)++;
   }
   return (erreur_t) {.code = SUCCESS};
}

static erreur_t decode_coef_AC_subsequent_scan(bitstream_t *bs, img_t *img, huffman_tree_t *symb_decode, blocl16_t *sortie, uint64_t *sortie_id, uint16_t *skip_bloc) {
   if (symb_decode->symb == 0xf0) {  // ZRL
      erreur_t err = skip_16_coef_AC_subsequent_scan(bs, img, sortie, sortie_id);
      if (err.code) return err;
   } else {
      uint8_t alpha = symb_decode->symb >> 4;
      uint8_t gamma = symb_decode->symb & 0b00001111;
      if (gamma == 0) {    // EOB
         if (alpha == 0) {
            *skip_bloc = 1;
         } else {
            uint16_t indice;
            erreur_t err = read_indice(bs, alpha, &indice);
            if (err.code) return err;
            *skip_bloc = indice + (1<<alpha);
         }
         erreur_t err = correction_eob(bs, img, sortie, sortie_id);
         if (err.code) return err;
         return (erreur_t) {.code=SUCCESS};
      } else if (gamma == 1) {  // 0x alpha 1
         erreur_t err = skip_n_coef_AC_subsequent_scan(bs, img, alpha, sortie, sortie_id);
         if (err.code) return err;
      } else {
         return (erreur_t) {.code = ERR_AC_BAD, .com = "En progressif les AC qui ne sont pas sur le premier scan doivent être 0xRRRRSSSS avec SSSS=0 ou 1", .must_free = false};
      }
   }
   *skip_bloc = 0;
   return (erreur_t) {.code=SUCCESS};
}

static erreur_t get_huffman_symbole(bitstream_t *bs, huffman_tree_t **ht, bool *code_que_un) {
   if (code_que_un != NULL) *code_que_un = true;
   while (true) {
      uint8_t bit;
      erreur_t err = read_bit(bs, &bit);
      if (err.code) return err;
      // on regarde si le bit courant est 0 ou 1
      *ht = (*ht)->fils[bit];
      if (code_que_un != NULL && bit == 0) *code_que_un = false;
      // Si on a atteint une feuille
      if ((*ht)->fils[1] == NULL && (*ht)->fils[0] == NULL) {
         return (erreur_t) {.code = SUCCESS};
      }
   }
}

static erreur_t decode_coef_DC(bitstream_t *bs, img_t *img, huffman_tree_t* ht, blocl16_t *sortie) {
   if (img->other->ah != 0) {  // Progressif scan ultérieur au premier
      if (img->other->ah - img->other->al != 1) {
         return (erreur_t) {.code = ERR_DIFF_AH_AL, "La différence entre ah et al devrait être 1", .must_free = false};
      }
      erreur_t err = decode_coef_DC_subsequent_scan(bs, img, sortie->data);
      if (err.code) return err;
   } else {  // Premier scan
      // On décode le symbole de Huffman puis on décode le coefficient
      huffman_tree_t *symb_decode = ht;
      bool code_que_un = true;
      erreur_t err = get_huffman_symbole(bs, &symb_decode, &code_que_un);
      if (code_que_un) {
         return (erreur_t) {.code=ERR_HUFF_CODE_1, .com="Le code de huffman avec que des 1 est utilisé\n", .must_free = false};
      }
      err = decode_coef_DC_first_scan(bs, img, symb_decode, sortie->data);
      if (err.code) return err;
   }
   return (erreur_t) {.code = SUCCESS};
}

static erreur_t decode_list_coef_AC(bitstream_t *bs, img_t *img, huffman_tree_t* ht, blocl16_t *sortie, uint16_t *skip_bloc) {
   uint64_t sortie_id = img->other->ss; // indice de où on en est dans sortie (le coefficient en cours de traitement)
   *skip_bloc = 0;
   while (sortie_id <= img->other->se) {
      huffman_tree_t *symb_decode = ht;
      erreur_t err = get_huffman_symbole(bs, &symb_decode, NULL);
      if (err.code) return err;
      
      if (img->other->ah == 0) {
         err = decode_coef_AC_first_scan(bs, img, symb_decode, sortie, &sortie_id, skip_bloc);
      } else {
         if (img->other->ah - img->other->al != 1) {
            return (erreur_t) {.code = ERR_DIFF_AH_AL, "La différence entre ah et al devrait être 1", .must_free = false};
         }
         err = decode_coef_AC_subsequent_scan(bs, img, symb_decode, sortie, &sortie_id, skip_bloc);
      }
      if (err.code) return err;
      if (*skip_bloc != 0) break;
      symb_decode = ht;
   }
   return (erreur_t) {.code = SUCCESS};
}

static erreur_t decode_bloc_acdc_baseline(bitstream_t *bs, img_t *img, huffman_tree_t *hdc, huffman_tree_t *hac, blocl16_t *sortie, int16_t *dc_prec, uint16_t *skip_bloc) {
   // Un coefficient DC puis 63 coefficients AC
   
   erreur_t err = decode_coef_DC(bs, img, hdc, sortie);
   if (err.code) return err;

   sortie->data[0] += *dc_prec;
   *dc_prec = sortie->data[0];
   
   img->other->ss = 1;
   err = decode_list_coef_AC(bs, img, hac, sortie, skip_bloc);
   img->other->ss = 0;
   if (err.code) return err;
   return (erreur_t) {.code = SUCCESS};
}

static erreur_t decode_bloc_acdc_progressif(bitstream_t *bs, img_t *img, huffman_tree_t *hdc, huffman_tree_t *hac, blocl16_t *sortie, int16_t *dc_prec, uint16_t *skip_bloc) {
   // Si ss = 0 alors on décode seulement DC sinon on décode seulement AC
   if (img->other->ss == 0) {
      erreur_t err = decode_coef_DC(bs, img, hdc, sortie);
      if (err.code) return err;

      if (img->other->ah == 0) {
         sortie->data[0] += *dc_prec;
      }
      *dc_prec = sortie->data[0];
   } else {
      erreur_t err = decode_list_coef_AC(bs, img, hac, sortie, skip_bloc);
      if (err.code) return err;
   }
   return (erreur_t) {.code = SUCCESS};
}

erreur_t decode_bloc_acdc(bitstream_t *bs, img_t *img, huffman_tree_t *hdc, huffman_tree_t *hac, blocl16_t *sortie, int16_t *dc_prec, uint16_t *skip_bloc) {
   if (img->section->num_sof == 0) return decode_bloc_acdc_baseline(bs, img, hdc, hac, sortie, dc_prec, skip_bloc);
   else if (img->section->num_sof == 2) return decode_bloc_acdc_progressif(bs, img, hdc, hac, sortie, dc_prec, skip_bloc);
   else return (erreur_t) {.code = ERR_SOF_BAD, .com = "Seulement le baseline et le progressif sont suportés", .must_free = false};
}
