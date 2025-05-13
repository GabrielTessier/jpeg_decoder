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
static int16_t read_val_from_magnitude(FILE *file, uint8_t magnitude, uint8_t *off, char *c);

// Retourne la valeur de la composante constante du bloc fréquentiel à l'adresse
// <file>+<off>, décodé grâce à l'arbre de Huffman <symb_decode>. <c> est la
// valeur de l'octet courant.
static int16_t decode_coef_DC(FILE *file, huffman_tree_t *symb_decode, uint8_t *off, char *c);

// Remplit <res>[<resi>] avec la composante non constante du bloc fréquentiel à
// l'adresse <file>+<off>, décodé grâce à l'arbre de Huffman <symb_decode>. <c>
// est la valeur de l'octet courant.
static void decode_coef_AC(FILE *file, huffman_tree_t *symb_decode, blocl16_t *sortie, uint64_t *resi, uint8_t s_start, uint8_t s_end, uint8_t *off, char *c);

// Retourne un tableau contenant les coefficients (AC ou DC selon <type>)
// décodés à l'adresse <file>+<off> à l'aide de l'arbre de Huffman <ht>
static void decode_list_coef(huffman_tree_t* ht, FILE* file, blocl16_t *sortie, uint8_t *off, enum acdc_e type, uint8_t s_start, uint8_t s_end);



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
      erreur("Pas de 0x00 après un 0xff (Pas bien !!)\n");
    }
  }
  char c = fgetc(file);
  return c;
}

static int16_t read_val_from_magnitude(FILE* file, uint8_t magnitude, uint8_t *off, char *c) {
  if (magnitude + *off < 8) {
    // On récupère la suite de bit sous forme de int puis on incrémente i
    uint16_t indice = (*c>>(7-*off-magnitude)) & ((1<<magnitude)-1);
    *off += magnitude;
    return get_val_from_magnitude(magnitude, indice);
  } else {
    // Si la valeur à lire est sur deux char
    // On lit les bits de poids fort puis ceux de poids faible
    uint16_t indice = (*c & ((1<<(7-*off))-1))<<(magnitude-(7-*off));
    //c = fgetc(file);
    *c = my_getc(file, *c);
    if (magnitude + *off < 16) {
      indice += (*c&((1<<8)-1))>>(8-(magnitude-(7-*off)));
      *off = magnitude-(7-*off)-1;
      return get_val_from_magnitude(magnitude, indice);
    } else {
      indice += (*c&((1<<8)-1))<<(magnitude-(7-*off)-8);
      *c = my_getc(file, *c);
      indice += (*c&((1<<8)-1))>>(8-(magnitude-(7-*off)-8));
      *off = (magnitude-(7-*off)-8)-1;
      return get_val_from_magnitude(magnitude, indice);
    }
  }
}

static int16_t decode_coef_DC(FILE *file, huffman_tree_t *symb_decode, uint8_t *off, char *c) {
  return read_val_from_magnitude(file, symb_decode->symb, off, c);
}

static void decode_coef_AC(FILE *file, huffman_tree_t *symb_decode, blocl16_t *sortie, uint64_t *resi, uint8_t s_start, uint8_t s_end, uint8_t *off, char *c) {
  switch (symb_decode->symb) {
  case (uint8_t) 0x00:
    *resi = 64;
    break;
  case (uint8_t) 0xf0:
    *resi += 16;
    break;
  default:
    uint8_t alpha = symb_decode->symb >> 4;
    uint8_t gamma = symb_decode->symb & 0b00001111;
    if (gamma == 0) erreur("Code invalide pour AC : %x", symb_decode->symb);
    *resi += alpha;
    sortie->data[(*resi)] = read_val_from_magnitude(file, gamma, off, c);
    (*resi)++;
    break;
  }
}

static void decode_list_coef(huffman_tree_t* ht, FILE* file, blocl16_t *sortie, uint8_t *off, enum acdc_e type, uint8_t s_start, uint8_t s_end) {
  uint64_t resi = s_start; // indice de où on en est dans res
  huffman_tree_t* symb_decode = ht; // où on en est dans l'arbre de huffman
  // On lit char par char mais on traite bit par bit
  uint8_t i = *off;
  char c = fgetc(file);
  bool code_que_un = true;
  while (true) {
    while (i<8) {
      // on regarde si le bit courant est 0 ou 1
      if (((c>>(7-i)) & 1) == 1) {
	symb_decode = symb_decode->fils[1];
      } else {
	symb_decode = symb_decode->fils[0];
	code_que_un = false;
      }
      // Si on a atteint une fauille
      if (symb_decode->fils[1] == NULL && symb_decode->fils[0] == NULL) {
	if (type == DC) {
	  if (code_que_un) {
	    erreur("Le code de huffman avec que des 1 est utilisé\n");
	  }
	  sortie->data[resi] = decode_coef_DC(file, symb_decode, &i, &c);
	  code_que_un = true;
	  resi++;
	} else {
	   decode_coef_AC(file, symb_decode, sortie, &resi, s_start, s_end, &i, &c);
	}
	symb_decode = ht;
	if (resi > s_end) break;
      }
      i++;
    }
    if (resi > s_end) break;
    c = my_getc(file, c);
    i = 0;
  }
  *off = i+1;
}

void decode_bloc_acdc(FILE *fichier, huffman_tree_t *hdc, huffman_tree_t *hac, blocl16_t *sortie, uint8_t s_start, uint8_t s_end, int16_t *dc_prec, uint8_t *off) {
   if (s_start == 0) {
      decode_list_coef(hdc, fichier, sortie, off, DC, 0, 0);
      sortie->data[0] += *dc_prec;
      *dc_prec = sortie->data[0];
      fseek(fichier, -1, SEEK_CUR);
      s_start = 1;
   }
   decode_list_coef(hac, fichier, sortie, off, AC, s_start, s_end);
   fseek(fichier, -1, SEEK_CUR);
}
