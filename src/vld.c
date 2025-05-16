#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include <utils.h>
#include <vld.h>
#include <img.h>
#include <erreur.h>

// Retourne la valeur correspondant à <indice> sachant la magnitude <magnitude>.
static int16_t get_val_from_magnitude(uint16_t magnitude, uint16_t indice);

// Retourne le prochain octet de <file> en vérifiant que si <old>
// le précédent octet valait 0xff, l'octet lu est bien 0x00.
static erreur_t my_getc(FILE *file, char *c);

// Retourne la valeur obtenue en lisant <magnitude> bits à l'adresse
// <file>+<off>. <c> est la valeur de l'octet courant.
static erreur_t read_val_from_magnitude(FILE* file, uint8_t magnitude, uint8_t *off, char *c, int16_t *val);
static erreur_t read_indice(FILE* file, uint8_t nb_bit, uint8_t *off, char *c, uint16_t *indice);

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

static erreur_t my_getc(FILE* file, char *c) {
   if (*c == (char) 0xff) {
      *c = fgetc(file);
      if (*c != (char) 0x00) {
	 char *str = malloc(80);
	 sprintf(str, "Pas de 0x00 après un 0xff (Pas bien !!) %lx\n", ftell(file)-1);
	 return (erreur_t) {.code = ERR_0XFF00, str};
      }
   }
   *c = fgetc(file);
   return (erreur_t) {.code = SUCCESS};
}

static erreur_t read_indice(FILE* file, uint8_t nb_bit, uint8_t *off, char *c, uint16_t *indice) {
   *indice = 0;
   while (nb_bit > 0) {
      (*off)++;
      if ((*off) == 8) {
	 *off = 0;
	 erreur_t err = my_getc(file, c);
	 if (err.code) return err;
      }
      *indice = *indice << 1;
      *indice += (((*c)>>(7-(*off))) & 1);
      nb_bit--;
   }
   return (erreur_t) {.code = SUCCESS};
}

static erreur_t read_val_from_magnitude(FILE* file, uint8_t magnitude, uint8_t *off, char *c, int16_t *val) {
   uint16_t indice;
   erreur_t err = read_indice(file, magnitude, off, c, &indice);
   if (err.code) return err;
   *val = get_val_from_magnitude(magnitude, indice);
   return (erreur_t) {.code = SUCCESS};
}

static erreur_t get_bit(FILE *file, uint8_t *off, char *c, uint8_t *bit) {
   (*off)++;
   if ((*off) == 8) {
      *off = 0;
      erreur_t err = my_getc(file, c);
      if (err.code) return err;
   }
   *bit = (((*c)>>(7-(*off))) & 1);
   return (erreur_t) {.code = SUCCESS};
}

static erreur_t decode_coef_DC_first_scan(FILE *file, img_t *img, huffman_tree_t *symb_decode, uint8_t *off, char *c, int16_t *coef_dc) {
   const uint8_t al = img->other->al;
   int16_t val;
   erreur_t err = read_val_from_magnitude(file, symb_decode->symb, off, c, &val);
   if (err.code) return err;
   *coef_dc = val*(1<<al);
   return (erreur_t) {.code = SUCCESS};
}

static erreur_t decode_coef_DC_subsequent_scan(FILE *file, img_t *img, uint8_t *off, char *c, int16_t *coef_dc) {
   const uint8_t al = img->other->al;
   uint8_t bit;
   erreur_t err = get_bit(file, off, c, &bit);
   if (err.code) return err;
   *coef_dc |= ((int16_t)bit)<<al;
   return (erreur_t) {.code = SUCCESS};
}

static erreur_t decode_coef_AC_first_scan(FILE *file, img_t *img, huffman_tree_t *symb_decode, blocl16_t *sortie, uint64_t *resi, uint8_t *off, char *c, uint16_t *skip_bloc) {
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
	       return (erreur_t) {.code=ERR_AC_BAD, .com=str};
	    } else if (num_sof == 2) {
	       uint16_t indice;
	       erreur_t err = read_indice(file, alpha, off, c, &indice);
	       if (err.code) return err;
	       *skip_bloc = indice + (1<<alpha);
	       return (erreur_t) {.code=SUCCESS};
	    } else {
	       char *str = malloc(27);
	       sprintf(str, "Numéro sof invalide : %d", num_sof);
	       return (erreur_t) {.code=ERR_SOF_BAD, .com=str};
	    }
	 }
      } else {
	 *resi += alpha;
	 erreur_t err = read_val_from_magnitude(file, gamma, off, c, sortie->data + (*resi));
	 if (err.code) return err;
	 sortie->data[*resi] = sortie->data[*resi]*(1<<al);
	 (*resi)++;
      }
   }
   *skip_bloc = 0;
   return (erreur_t) {.code=SUCCESS};
}

static erreur_t skip_n_coef_AC_subsequent_scan(FILE *file, img_t *img, uint8_t n, uint8_t magnitude, blocl16_t *sortie, uint64_t *resi, uint8_t *off, char *c) {
   const uint8_t al = img->other->al;
   printf("resi : %ld, n : %d\n", *resi, n);
   int16_t val = 0;
   erreur_t err = read_val_from_magnitude(file, magnitude, off, c, &val);
   if (err.code) return err;
   int i=0;
   while (i<n) {
      if (sortie->data[*resi] != 0) {
	 uint8_t bit;
	 erreur_t err = get_bit(file, off, c, &bit);
	 if (err.code) return err;
	 if (bit == 1) sortie->data[*resi] |= (int16_t)1<<al;
      } else {
	 i++;
      }
      (*resi)++;
   }
   sortie->data[*resi] = val*(1<<al);
   (*resi)++;
   return (erreur_t) {.code = SUCCESS};
}

static erreur_t skip_16_coef_AC_subsequent_scan(FILE *file, img_t *img, blocl16_t *sortie, uint64_t *resi, uint8_t *off, char *c) {
   const uint8_t al = img->other->al;
   printf("resi : %ld, n : %d\n", *resi, 16);
   for (int i=0; i<16; i++) {
      if (sortie->data[*resi] != 0) {
	 uint8_t bit;
	 erreur_t err = get_bit(file, off, c, &bit);
	 if (err.code) return err;
	 if (bit == 1) sortie->data[*resi] |= (int16_t)1<<al;
      }
      (*resi)++;
   }
   return (erreur_t) {.code = SUCCESS};
}

static erreur_t correction_eob(FILE *file, img_t *img, blocl16_t *sortie, uint64_t *resi, uint8_t *off, char *c) {
   const uint8_t al = img->other->al;
   while (*resi <= img->other->se) {
      if (sortie->data[*resi] != 0) {
	 uint8_t bit;
	 erreur_t err = get_bit(file, off, c, &bit);
	 if (err.code) return err;
	 if (bit == 1) sortie->data[*resi] |= (int16_t)1<<al;
      }
      (*resi)++;
   }
   return (erreur_t) {.code = SUCCESS};
}

static erreur_t decode_coef_AC_subsequent_scan(FILE *file, img_t *img, huffman_tree_t *symb_decode, blocl16_t *sortie, uint64_t *resi, uint8_t *off, char *c, uint16_t *skip_bloc) {
   const uint8_t num_sof = img->section->num_sof;
   if (symb_decode->symb == 0xf0) {  // RLE
      erreur_t err = skip_16_coef_AC_subsequent_scan(file, img, sortie, resi, off, c);
      if (err.code) return err;
   } else {
      uint8_t alpha = symb_decode->symb >> 4;
      uint8_t gamma = symb_decode->symb & 0b00001111;
      if (gamma == 0) {
	 if (alpha == 0) {
	    *skip_bloc = 1;
	    erreur_t err = correction_eob(file, img, sortie, resi, off, c);
	    if (err.code) return err;
	    return (erreur_t) {.code=SUCCESS};
	 } else {
	    if (num_sof == 0) {
	       char *str = malloc(80);
	       sprintf(str, "Code invalide pour AC (%x) car mode baseline", symb_decode->symb);
	       return (erreur_t) {.code=ERR_AC_BAD, .com=str};
	    } else if (num_sof == 2) {
	       uint16_t indice;
	       erreur_t err = read_indice(file, alpha, off, c, &indice);
	       if (err.code) return err;
	       *skip_bloc = indice + (1<<alpha);
	       err = correction_eob(file, img, sortie, resi, off, c);
	       if (err.code) return err;
	       return (erreur_t) {.code=SUCCESS};
	    } else {
	       char *str = malloc(27);
	       sprintf(str, "Numéro sof invalide : %d", num_sof);
	       return (erreur_t) {.code=ERR_SOF_BAD, .com=str};
	    }
	 }
      } else if (gamma == 1) {
	 erreur_t err = skip_n_coef_AC_subsequent_scan(file, img, alpha, gamma, sortie, resi, off, c);
	 if (err.code) return err;
      } else {
	 return (erreur_t) {.code = ERR_AC_BAD, .com = "En progressif les AC qui ne sont pas sur le premier scan doivent être 0xRRRRSSSS avec SSSS=0 ou 1"};
      }
   }
   *skip_bloc = 0;
   return (erreur_t) {.code=SUCCESS};
}

static erreur_t decode_list_coef_DC(FILE* file, img_t *img, huffman_tree_t* ht, blocl16_t *sortie, uint8_t *off) {
   erreur_t succ = {.code=SUCCESS};
   uint64_t resi = img->other->ss; // indice de où on en est dans res
   huffman_tree_t* symb_decode = ht; // où on en est dans l'arbre de huffman
   // On lit char par char mais on traite bit par bit
   uint8_t i = *off;
   char c = fgetc(file);
   bool code_que_un = true;
   while (true) {
      while (i<8) {
	 if (img->other->ah != 0) {
	    if (img->other->ah - img->other->al != 1) {
	       return (erreur_t) {.code = ERR_DIFF_AH_AL, "La différence entre ah et al devrait être 1"};
	    }
	    erreur_t err = decode_coef_DC_subsequent_scan(file, img, &i, &c, sortie->data + resi);
	    if (err.code) return err;
	    resi++;
	 } else {
	    // on regarde si le bit courant est 0 ou 1
	    if (((c>>(7-i)) & 1) == 1) {
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
	       erreur_t err = decode_coef_DC_first_scan(file, img, symb_decode, &i, &c, sortie->data + resi);
	       if (err.code) return err;
	       code_que_un = true;
	       resi++;
	       symb_decode = ht;
	    }
	 }
	 if (resi > img->other->se) break;
	 i++;
      }
      if (resi > img->other->se) break;
      erreur_t err = my_getc(file, &c);
      if (err.code) return err;
      i = 0;
   }
   *off = i+1;
   return succ;
}

static erreur_t decode_list_coef_AC(FILE* file, img_t *img, huffman_tree_t* ht, blocl16_t *sortie, uint8_t *off, uint16_t *skip_bloc) {
   erreur_t succ = {.code=SUCCESS};
   uint64_t resi = img->other->ss; // indice de où on en est dans res
   huffman_tree_t* symb_decode = ht; // où on en est dans l'arbre de huffman
   // On lit char par char mais on traite bit par bit
   uint8_t i = *off;
   char c = fgetc(file);
   *skip_bloc = 0;
   while (true) {
      while (i<8) {
	 // on regarde si le bit courant est 0 ou 1
	 if (((c>>(7-i)) & 1) == 1) {
	    symb_decode = symb_decode->fils[1];
	 } else {
	    symb_decode = symb_decode->fils[0];
	 }
	 // Si on a atteint une feuille
	 if (symb_decode->fils[1] == NULL && symb_decode->fils[0] == NULL) {
	    erreur_t err;
	    if (img->other->ah == 0) {
	       err = decode_coef_AC_first_scan(file, img, symb_decode, sortie, &resi, &i, &c, skip_bloc);
	    } else {
	       if (img->other->ah - img->other->al != 1) {
		  return (erreur_t) {.code = ERR_DIFF_AH_AL, "La différence entre ah et al devrait être 1"};
	       }
	       err = decode_coef_AC_subsequent_scan(file, img, symb_decode, sortie, &resi, &i, &c, skip_bloc);
	    }
	    if (err.code) return err;
	    if (*skip_bloc != 0) resi = 64;
	    symb_decode = ht;
	    if (resi > img->other->se) break;
	 }
	 i++;
      }
      if (resi > img->other->se) break;
      erreur_t err = my_getc(file, &c);
      if (err.code) return err;
      i = 0;
   }
   *off = i+1;
   return succ;
}

erreur_t decode_bloc_acdc(FILE *fichier, img_t *img, huffman_tree_t *hdc, huffman_tree_t *hac, blocl16_t *sortie, int16_t *dc_prec, uint8_t *off, uint16_t *skip_bloc) {
   uint8_t s_start = img->other->ss;
   uint8_t s_start_tmp = s_start;
   uint8_t s_end = img->other->se;
   if (s_start == 0) {
      img->other->se = 0;
      erreur_t err = decode_list_coef_DC(fichier, img, hdc, sortie, off);
      if (err.code) return err;
      img->other->se = s_end;
      
      sortie->data[0] += *dc_prec;
      *dc_prec = sortie->data[0];
      fseek(fichier, -1, SEEK_CUR);
      s_start = 1;
   }
   if (s_start <= s_end) {
      img->other->ss = s_start;
      erreur_t err = decode_list_coef_AC(fichier, img, hac, sortie, off, skip_bloc);
      if (err.code) return err;
      img->other->ss = s_start_tmp;
      fseek(fichier, -1, SEEK_CUR);
      return (erreur_t) {.code = SUCCESS};
   }
   *skip_bloc = 0;
   return (erreur_t) {.code = SUCCESS};
}
