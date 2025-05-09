#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "vld.h"

int16_t get_val_from_magnitude(uint16_t magnitude, uint16_t indice) {
  if (magnitude == 0) return 0;
  int16_t min = 1<<(magnitude-1);
  int16_t max = (min<<1)-1;
  if (indice < min) return indice-max;  // Négatif
  return indice;
}

char my_getc(FILE* file, char old) {
  if (old == (char) 0xff) {
    old = fgetc(file);
    if (old != 0) {
      fprintf(stderr, "Erreur : Pas de 0x00 après un 0xff (Pas bien !!)\n");
      exit(EXIT_FAILURE);
    }
  }
  char c = fgetc(file);
  return c;
}

blocl16_t *decode_bloc_acdc(FILE *fichier, huffman_tree_t *hdc, huffman_tree_t *hac, int16_t *dc_prec, uint8_t *off) {
  int16_t *sousdc = decode_list_coef(hdc, fichier, off, DC);
  int16_t dc = sousdc[0];
  dc += *dc_prec; // calcul du coef à partir de la différence
  *dc_prec = dc;

  fseek(fichier, -1, SEEK_CUR);
  int16_t *ac = decode_list_coef(hac, fichier, off, AC);

  fseek(fichier, -1, SEEK_CUR);
  // écriture des DC, AC
  blocl16_t * bloc = (blocl16_t*) malloc(sizeof(blocl16_t));
  bloc->data[0] = dc;
  //memcpy(bloc->data+1, ac, 63*sizeof(int16_t));
  for (int i=0; i<63; i++) bloc->data[i+1] = ac[i];
  free(sousdc);
  free(ac);
  return bloc;
}

int16_t read_val_from_magnitude(FILE* file, uint8_t magnitude, uint8_t *off, char *c) {
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

int16_t decode_coef_DC(FILE *file, huffman_tree_t *symb_decode, uint8_t *off, char *c) {
  return read_val_from_magnitude(file, symb_decode->symb, off, c);
}

void decode_coef_AC(FILE *file, huffman_tree_t *symb_decode, int16_t *res,
                    uint64_t *resi, uint8_t *off, char *c) {
  switch (symb_decode->symb) {
  case (uint8_t) 0x00:
    *resi = 63;
    break;
  case (uint8_t) 0xf0:
    *resi += 16;
    break;
  default:
    uint8_t alpha = symb_decode->symb >> 4;
    uint8_t gamma = symb_decode->symb & 0b00001111;
    *resi += alpha;
    res[*resi] = read_val_from_magnitude(file, gamma, off, c);
    (*resi)++;
    break;
  }
}

int16_t *decode_list_coef(huffman_tree_t* ht, FILE* file, uint8_t *off, enum acdc_e type) {
  uint64_t size = 0;
  if (type==AC) size = 63;
  else size = 1;
  int16_t* res = (int16_t*) calloc(size, sizeof(int16_t)); // tableau contenant les dc décodé
  uint64_t resi = 0; // indice de où on en est dans res
  huffman_tree_t* symb_decode = ht; // où on en est dans l'arbre de huffman
  // On lit char par char mais on traite bit par bit
  uint8_t i = *off;
  char c = fgetc(file);
  bool code_que_un = true;
  while (true) {
    while (i<8) {
      // on regarde si le bit courant est 0 ou 1
      if (((c>>(7-i)) & 1) == 1) {
	symb_decode = symb_decode->droit;
      } else {
	symb_decode = symb_decode->gauche;
	code_que_un = false;
      }
      // Si on a atteint une fauille
      if (symb_decode->droit == NULL && symb_decode->gauche == NULL) {
	if (type == DC) {
	  if (code_que_un) {
	    fprintf(stderr, "Le code de huffman avec que des 1 est utilisé\n");
	    exit(EXIT_FAILURE);
	  }
	  res[resi] = decode_coef_DC(file, symb_decode, &i, &c);
	  code_que_un = true;
	  resi++;
	} else {
	  decode_coef_AC(file, symb_decode, res, &resi, &i, &c);
	}
	symb_decode = ht;
	if (resi >= size) break;
      }
      i++;
    }
    if (resi >= size) break;
    c = my_getc(file, c);
    i = 0;
  }
  *off = i+1;
  return res;
}
