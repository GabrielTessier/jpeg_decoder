#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include <utils.h>
#include <vld.h>
#include <img.h>
#include <erreur.h>
#include <bitstream.h>

// Retourne la valeur correspondant à <indice> sachant la magnitude <magnitude>.
//static int16_t get_val_from_magnitude(uint16_t magnitude, uint16_t indice);

// Retourne la valeur obtenue en lisant <magnitude> bits à l'adresse
// <file>+<off>. <c> est la valeur de l'octet courant.
//static erreur_t read_val_from_magnitude(FILE* file, uint8_t magnitude, uint8_t *off, char *c, int16_t *val);
//static erreur_t read_indice(FILE* file, uint8_t nb_bit, uint8_t *off, char *c, uint16_t *indice);

// Retourne la valeur de la composante constante du bloc fréquentiel à l'adresse
// <file>+<off>, décodé grâce à l'arbre de Huffman <symb_decode>. <c> est la
// valeur de l'octet courant.
//static erreur_t decode_coef_DC(FILE *file, huffman_tree_t *symb_decode, uint8_t *off, char *c, int16_t *coef_dc);

// Remplit <res>[<resi>] avec la composante non constante du bloc fréquentiel à
// l'adresse <file>+<off>, décodé grâce à l'arbre de Huffman <symb_decode>. <c>
// est la valeur de l'octet courant.
//static erreur_t decode_coef_AC(FILE *file, uint8_t num_sof, huffman_tree_t *symb_decode, blocl16_t *sortie, uint64_t *resi, uint8_t *off, char *c, uint16_t *skip_bloc);

// Retourne un tableau contenant les coefficients (AC ou DC selon <type>)
// décodés à l'adresse <file>+<off> à l'aide de l'arbre de Huffman <ht>
//static erreur_t decode_list_coef(FILE* file, img_t *img, huffman_tree_t* ht, blocl16_t *sortie, uint8_t *off, enum acdc_e type, uint16_t *skip_bloc);


static int16_t get_val_from_magnitude(uint16_t magnitude, uint16_t indice) {
   if (magnitude == 0) return 0;
   int16_t min = 1<<(magnitude-1);
   int16_t max = (min<<1)-1;
   if (indice < min) return indice-max;  // Négatif
   return indice;
}

static erreur_t read_indice(bitstream_t *bs, uint8_t nb_bit, uint16_t *indice) {
   *indice = 0;
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
   erreur_t err = read_val_from_magnitude(bs, symb_decode->symb, &val);
   if (err.code) return err;
   *coef_dc = val*(1<<img->other->al);
   return (erreur_t) {.code = SUCCESS};
}

static erreur_t decode_coef_DC_subsequent_scan(bitstream_t *bs, img_t *img, int16_t *coef_dc) {
   uint8_t bit;
   erreur_t err = read_bit(bs, &bit);
   if (err.code) return err;
   *coef_dc |= ((int16_t)bit)<<img->other->al;
   return (erreur_t) {.code = SUCCESS};
}

static erreur_t decode_coef_AC_first_scan(bitstream_t *bs, img_t *img, huffman_tree_t *symb_decode, blocl16_t *sortie, uint64_t *resi, uint16_t *skip_bloc) {
   const uint8_t num_sof = img->section->num_sof;
   const uint8_t al = img->other->al;
   if (symb_decode->symb == 0xf0) {  // RLE
      *resi += 16;
   } else {
      uint8_t alpha = symb_decode->symb >> 4;
      uint8_t gamma = symb_decode->symb & 0b00001111;
      if (gamma == 0) {
	 if (alpha == 0) {
	    *skip_bloc = 1;
	    return (erreur_t) {.code=SUCCESS};
	 } else {
	    if (num_sof == 0) {
	       char *str = malloc(80);
	       sprintf(str, "Code invalide pour AC (%x) car mode baseline", symb_decode->symb);
	       return (erreur_t) {.code=ERR_AC_BAD, .com=str, .must_free = true};
	    } else if (num_sof == 2) {
	       uint16_t indice;
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
      } else {
	 *resi += alpha;
	 erreur_t err = read_val_from_magnitude(bs, gamma, sortie->data + (*resi));
	 if (err.code) return err;
	 sortie->data[*resi] = sortie->data[*resi]*(1<<al);
	 (*resi)++;
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
   while (coefs[*indice_coef] != 0) {
      erreur_t err = correction_coef(bs, img, &(coefs[*indice_coef]));
      if (err.code) return err;
      (*indice_coef)++;
   }
   return (erreur_t) {.code = SUCCESS};
}

static erreur_t skip_n_coef_AC_subsequent_scan(bitstream_t *bs, img_t *img, uint8_t n, blocl16_t *sortie, uint64_t *resi) {
   int16_t val = 0;
   erreur_t err = read_val_from_magnitude(bs, 1, &val);
   if (err.code) return err;

   err = correction_n_coef_jusqua_zero(bs, img, n, sortie->data, resi);
   if (err.code) return err;
   
   sortie->data[*resi] = val*(1<<img->other->al);
   (*resi)++;
   return (erreur_t) {.code = SUCCESS};
}

static erreur_t skip_16_coef_AC_subsequent_scan(bitstream_t *bs, img_t *img, blocl16_t *sortie, uint64_t *resi) {
   return correction_n_coef(bs, img, 16, sortie->data, resi);
}

erreur_t correction_eob(bitstream_t *bs, img_t *img, blocl16_t *sortie, uint64_t *resi) {
   while (*resi <= img->other->se) {
      if (sortie->data[*resi] != 0) {
	 erreur_t err = correction_coef(bs, img, &(sortie->data[*resi]));
	 if (err.code) return err;
      }
      (*resi)++;
   }
   return (erreur_t) {.code = SUCCESS};
}

static erreur_t decode_coef_AC_subsequent_scan(bitstream_t *bs, img_t *img, huffman_tree_t *symb_decode, blocl16_t *sortie, uint64_t *resi, uint16_t *skip_bloc) {
   if (symb_decode->symb == 0xf0) {  // RLE
      erreur_t err = skip_16_coef_AC_subsequent_scan(bs, img, sortie, resi);
      if (err.code) return err;
   } else {
      uint8_t alpha = symb_decode->symb >> 4;
      uint8_t gamma = symb_decode->symb & 0b00001111;
      if (gamma == 0) {
	 if (alpha == 0) {
	    *skip_bloc = 1;
	 } else {
	    uint16_t indice;
	    erreur_t err = read_indice(bs, alpha, &indice);
	    if (err.code) return err;
	    *skip_bloc = indice + (1<<alpha);
	 }
	 erreur_t err = correction_eob(bs, img, sortie, resi);
	 if (err.code) return err;
	 return (erreur_t) {.code=SUCCESS};
      } else if (gamma == 1) {
	 erreur_t err = skip_n_coef_AC_subsequent_scan(bs, img, alpha, sortie, resi);
	 if (err.code) return err;
      } else {
	 return (erreur_t) {.code = ERR_AC_BAD, .com = "En progressif les AC qui ne sont pas sur le premier scan doivent être 0xRRRRSSSS avec SSSS=0 ou 1"};
      }
   }
   *skip_bloc = 0;
   return (erreur_t) {.code=SUCCESS};
}

static erreur_t decode_list_coef_DC(bitstream_t *bs, img_t *img, huffman_tree_t* ht, blocl16_t *sortie) {
   uint64_t resi = img->other->ss; // indice de où on en est dans res
   huffman_tree_t* symb_decode = ht; // où on en est dans l'arbre de huffman
   // On lit char par char mais on traite bit par bit
   bool code_que_un = true;
   while (resi <= img->other->se) {
      if (img->other->ah != 0) {
	 if (img->other->ah - img->other->al != 1) {
	    return (erreur_t) {.code = ERR_DIFF_AH_AL, "La différence entre ah et al devrait être 1"};
	 }
	 erreur_t err = decode_coef_DC_subsequent_scan(bs, img, sortie->data + resi);
	 if (err.code) return err;
	 resi++;
      } else {
	 uint8_t bit;
	 erreur_t err = read_bit(bs, &bit);
	 if (err.code) return err;
	 // on regarde si le bit courant est 0 ou 1
	 if (bit == 1) {
	    symb_decode = symb_decode->fils[1];
	 } else {
	    symb_decode = symb_decode->fils[0];
	    code_que_un = false;
	 }
	 // Si on a atteint une feuille
	 if (symb_decode->fils[1] == NULL && symb_decode->fils[0] == NULL) {
	    if (code_que_un) {
	       return (erreur_t) {.code=ERR_HUFF_CODE_1, .com="Le code de huffman avec que des 1 est utilisé\n"};
	    }
	    erreur_t err = decode_coef_DC_first_scan(bs, img, symb_decode, sortie->data + resi);
	    if (err.code) return err;
	    code_que_un = true;
	    resi++;
	    symb_decode = ht;
	 }
      }
   }
   return (erreur_t) {.code = SUCCESS};
}

static erreur_t decode_list_coef_AC(bitstream_t *bs, img_t *img, huffman_tree_t* ht, blocl16_t *sortie, uint16_t *skip_bloc) {
   uint64_t resi = img->other->ss; // indice de où on en est dans res
   huffman_tree_t* symb_decode = ht; // où on en est dans l'arbre de huffman
   // On lit char par char mais on traite bit par bit
   *skip_bloc = 0;
   while (resi <= img->other->se) {
      uint8_t bit;
      erreur_t err = read_bit(bs, &bit);
      if (err.code) return err;
      // on regarde si le bit courant est 0 ou 1
      if (bit == 1) {
	 symb_decode = symb_decode->fils[1];
      } else {
	 symb_decode = symb_decode->fils[0];
      }
      // Si on a atteint une feuille
      if (symb_decode->fils[1] == NULL && symb_decode->fils[0] == NULL) {
	 erreur_t err;
	 if (img->other->ah == 0) {
	    err = decode_coef_AC_first_scan(bs, img, symb_decode, sortie, &resi, skip_bloc);
	 } else {
	    if (img->other->ah - img->other->al != 1) {
	       return (erreur_t) {.code = ERR_DIFF_AH_AL, "La différence entre ah et al devrait être 1"};
	    }
	    err = decode_coef_AC_subsequent_scan(bs, img, symb_decode, sortie, &resi, skip_bloc);
	 }
	 if (err.code) return err;
	 if (*skip_bloc != 0) break;
	 symb_decode = ht;
      }
   }
   return (erreur_t) {.code = SUCCESS};
}

static erreur_t decode_bloc_acdc_baseline(bitstream_t *bs, img_t *img, huffman_tree_t *hdc, huffman_tree_t *hac, blocl16_t *sortie, int16_t *dc_prec, uint16_t *skip_bloc) {
   img->other->se = 0;
   erreur_t err = decode_list_coef_DC(bs, img, hdc, sortie);
   img->other->se = 63;
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
   if (img->other->ss == 0) {
      erreur_t err = decode_list_coef_DC(bs, img, hdc, sortie);
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
   else return (erreur_t) {.code = ERR_SOF_BAD, .com = "Seulement le baseline et le progressif sont suportés"};
}
