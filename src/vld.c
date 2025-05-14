#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include <utils.h>
#include <vld.h>

// Retourne la valeur correspondant à <indice> sachant la magnitude <magnitude>.
static int16_t get_val_from_magnitude(uint16_t magnitude, uint16_t indice);

// Retourne le prochain octet de <file> en vérifiant que si <old>
// le précédent octet valait 0xff, l'octet lu est bien 0x00.
static char my_getc(FILE *file, char old);

// Retourne la valeur obtenue en lisant <magnitude> bits à l'adresse
// <file>+<off>. <c> est la valeur de l'octet courant.
static int16_t read_val_from_magnitude(FILE* file, uint8_t magnitude, uint8_t *off, char *c);
static uint16_t read_indice(FILE* file, uint8_t nb_bit, uint8_t *off, char *c);

// Retourne la valeur de la composante constante du bloc fréquentiel à l'adresse
// <file>+<off>, décodé grâce à l'arbre de Huffman <symb_decode>. <c> est la
// valeur de l'octet courant.
static int16_t decode_coef_DC(FILE *file, huffman_tree_t *symb_decode, uint8_t *off, char *c);

// Remplit <res>[<resi>] avec la composante non constante du bloc fréquentiel à
// l'adresse <file>+<off>, décodé grâce à l'arbre de Huffman <symb_decode>. <c>
// est la valeur de l'octet courant.
static uint16_t decode_coef_AC(FILE *file, uint8_t num_sof, huffman_tree_t *symb_decode, blocl16_t *sortie, uint64_t *resi, uint8_t *off, char *c);

// Retourne un tableau contenant les coefficients (AC ou DC selon <type>)
// décodés à l'adresse <file>+<off> à l'aide de l'arbre de Huffman <ht>
static uint16_t decode_list_coef(FILE* file, huffman_tree_t* ht, uint8_t num_sof, blocl16_t *sortie, uint8_t *off, enum acdc_e type, uint8_t s_start, uint8_t s_end);

bool stop = false;

static int16_t get_val_from_magnitude(uint16_t magnitude, uint16_t indice) {
   if (magnitude == 0) return 0;
   int16_t min = 1<<(magnitude-1);
   int16_t max = (min<<1)-1;
   if (indice < min) return indice-max;  // Négatif
   return indice;
}

static char my_getc(FILE* file, char old) {
   if (old == (char) 0xff) {
      old = fgetc(file);
      if (old != 0) {
	 //erreur("Pas de 0x00 après un 0xff (Pas bien !!) %x\n", ftell(file)-1);
	 stop = true;
      }
   }
   char c = fgetc(file);
   return c;
}

static uint16_t read_indice2(FILE* file, uint8_t nb_bit, uint8_t *off, char *c) {
   if (nb_bit + *off < 8) {
      // On récupère la suite de bit sous forme de int puis on incrémente i
      uint16_t indice = (*c>>(7-*off-nb_bit)) & ((1<<nb_bit)-1);
      *off += nb_bit;
      return indice;
   } else {
      // Si la valeur à lire est sur deux char
      // On lit les bits de poids fort puis ceux de poids faible
      uint16_t indice = (*c & ((1<<(7-*off))-1))<<(nb_bit-(7-*off));
      //c = fgetc(file);
      *c = my_getc(file, *c);
      if (stop) return 0;
      if (nb_bit + *off < 16) {
	 indice += (*c&((1<<8)-1))>>(8-(nb_bit-(7-*off)));
	 *off = nb_bit-(7-*off)-1;
	 return indice;
      } else {
	 indice += (*c&((1<<8)-1))<<(nb_bit-(7-*off)-8);
	 *c = my_getc(file, *c);
	 if (stop) return 0;
	 indice += (*c&((1<<8)-1))>>(8-(nb_bit-(7-*off)-8));
	 *off = (nb_bit-(7-*off)-8)-1;
	 return indice;
      }
   }
}

static uint16_t read_indice(FILE* file, uint8_t nb_bit, uint8_t *off, char *c) {
   uint16_t indice = 0;
   while (nb_bit > 0) {
      (*off)++;
      if ((*off) == 8) {
	 *off = 0;
	 *c = my_getc(file, *c);
	 if (stop) return 0;
      }
      indice = indice << 1;
      indice += (((*c)>>(7-(*off))) & 1);
      nb_bit--;
   }
   return indice;
}

static int16_t read_val_from_magnitude(FILE* file, uint8_t magnitude, uint8_t *off, char *c) {
   uint16_t indice = read_indice(file, magnitude, off, c);
   if (stop) return 0;
   return get_val_from_magnitude(magnitude, indice);
}

static int16_t decode_coef_DC(FILE *file, huffman_tree_t *symb_decode, uint8_t *off, char *c) {
   return read_val_from_magnitude(file, symb_decode->symb, off, c);
}

static uint16_t decode_coef_AC(FILE *file, uint8_t num_sof, huffman_tree_t *symb_decode, blocl16_t *sortie, uint64_t *resi, uint8_t *off, char *c) {
   if (symb_decode->symb == 0xf0) {  // RLE
      *resi += 16;
   } else {
      uint8_t alpha = symb_decode->symb >> 4;
      uint8_t gamma = symb_decode->symb & 0b00001111;
      if (gamma == 0) {
	 if (alpha == 0) {
	    return 1;
	 } else {
	    if (num_sof == 0) {
	       erreur("Code invalide pour AC (%x) car mode baseline", symb_decode->symb);
	    } else if (num_sof == 2) {
	       uint16_t indice = read_indice(file, alpha, off, c);
	       if (stop) return 0;
	       uint16_t skip_bloc = indice + (1<<alpha);
	       return skip_bloc;
	    } else {
	       erreur("Numéro sof invalide : %d", num_sof);
	    }
	 }
      } else {
	 *resi += alpha;
	 sortie->data[(*resi)] = read_val_from_magnitude(file, gamma, off, c);
	 if (stop) return 0;
	 (*resi)++;
      }
   }
   return 0;
}

static uint16_t decode_list_coef(FILE* file, huffman_tree_t* ht, uint8_t num_sof, blocl16_t *sortie, uint8_t *off, enum acdc_e type, uint8_t s_start, uint8_t s_end) {
   uint64_t resi = s_start; // indice de où on en est dans res
   huffman_tree_t* symb_decode = ht; // où on en est dans l'arbre de huffman
   // On lit char par char mais on traite bit par bit
   uint8_t i = *off;
   char c = fgetc(file);
   bool code_que_un = true;
   uint16_t skip_bloc = 0;
   while (true) {
      while (i<8) {
	 // on regarde si le bit courant est 0 ou 1
	 if (((c>>(7-i)) & 1) == 1) {
	    symb_decode = symb_decode->fils[1];
	 } else {
	    symb_decode = symb_decode->fils[0];
	    code_que_un = false;
	 }
	 // Si on a atteint une feuille
	 if (symb_decode->fils[1] == NULL && symb_decode->fils[0] == NULL) {
	    if (type == DC) {
	       if (code_que_un) {
		  erreur("Le code de huffman avec que des 1 est utilisé\n");
	       }
	       sortie->data[resi] = decode_coef_DC(file, symb_decode, &i, &c);
	       code_que_un = true;
	       resi++;
	    } else {
	       skip_bloc = decode_coef_AC(file, num_sof, symb_decode, sortie, &resi, &i, &c);
	       if (stop) return 0;
	       if (skip_bloc != 0) resi = 64;
	    }
	    symb_decode = ht;
	    if (resi > s_end) break;
	 }
	 i++;
      }
      if (resi > s_end) break;
      c = my_getc(file, c);
      if (stop) return 0;
      i = 0;
   }
   *off = i+1;
   return skip_bloc;
}

uint16_t decode_bloc_acdc(FILE *fichier, uint8_t num_sof, huffman_tree_t *hdc, huffman_tree_t *hac, blocl16_t *sortie, uint8_t s_start, uint8_t s_end, int16_t *dc_prec, uint8_t *off) {
   if (s_start == 0) {
      decode_list_coef(fichier, hdc, num_sof, sortie, off, DC, 0, 0);
      sortie->data[0] += *dc_prec;
      *dc_prec = sortie->data[0];
      fseek(fichier, -1, SEEK_CUR);
      s_start = 1;
   }
   if (s_start <= s_end) {
      uint16_t skip_bloc = decode_list_coef(fichier, hac, num_sof, sortie, off, AC, s_start, s_end);
      if (stop) return 0;
      fseek(fichier, -1, SEEK_CUR);
      return skip_bloc;
   }
   return 0;
}
