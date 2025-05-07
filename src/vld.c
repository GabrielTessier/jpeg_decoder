#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "vld.h"

int16_t get_val_from_magnitude(uint16_t magnitude, uint16_t indice) {
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

int16_t *decodeDC(huffman_tree_t* ht, FILE* file, uint64_t pos, uint8_t *off, uint64_t size) {
  int16_t diff = 0;
  fseek(file, pos, SEEK_SET);// On se place au début du code
  int16_t* res = (int16_t*) malloc(sizeof(int8_t)*size); // tableau contenant les dc décodé
  uint64_t resi = 0; // indice de où on en est dans res
  huffman_tree_t* symb_decode = ht; // où on en est dans l'arbre de huffman
  // On lit char par char mais on traite bit par bit
  int i = *off;
  char c = fgetc(file);
  //printf("DCDC : %x, %d\n", c, i);
  //if (c == (char)0x01 && i == 3) i = 2;
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
	if (code_que_un) {
	  fprintf(stderr, "Le code de huffman avec que des 1 est utilisé\n");
	  exit(EXIT_FAILURE);
	}
	code_que_un = true;
	// Si la valeur à lire est entièrement sur le char courant
	if (symb_decode->symb + i < 8) {
	  // On récupère la suite de bit sous forme de int puis on incrémente i
	  uint16_t indice = (c>>(7-i-symb_decode->symb)) & ((1<<symb_decode->symb)-1);
	  i += symb_decode->symb;
	  res[resi] = diff + get_val_from_magnitude(symb_decode->symb, indice);
	} else {
	  // Si la valeur à lire est sur deux char
	  // On lit les bits de poids fort puis ceux de poids faible
	  uint16_t indice = (c & ((1<<(7-i))-1))<<(symb_decode->symb-(7-i));
	  //c = fgetc(file);
	  c = my_getc(file, c);
	  if (symb_decode->symb + i < 16) {
	    indice += (c&((1<<8)-1))>>(8-(symb_decode->symb-(7-i)));
	    i = symb_decode->symb-(7-i)-1;
	    res[resi] = diff + get_val_from_magnitude(symb_decode->symb, indice);
	  } else {
	    indice += (c&((1<<8)-1))<<(symb_decode->symb-(7-i)-8);
	    c = my_getc(file, c);
	    indice += (c&((1<<8)-1))>>(8-(symb_decode->symb-(7-i)-8));
	    i = (symb_decode->symb-(7-i)-8)-1;
	    res[resi] = diff + get_val_from_magnitude(symb_decode->symb, indice);
	  }
	}
	diff = res[resi];
	resi++;
	symb_decode = ht;
	if (resi == size) break;
      }
      i++;
    }
    if (resi == size) break;
    //c = fgetc(file);
    c = my_getc(file, c);
    i = 0;
  }
  *off = i+1;
  return res;
}

int16_t *decodeAC(huffman_tree_t* ht, FILE* file, uint64_t pos, uint8_t *off) {
  fseek(file, pos, SEEK_SET);
  int16_t *res = (int16_t*) calloc(63, sizeof(int16_t));
  uint8_t resi = 0;
  
  huffman_tree_t* symb_decode = ht;
  int i = *off;
  char c = fgetc(file);
  while (true) {
    while (i<8) {
      // on regarde si le bit courant est 0 ou 1
      if (((c>>(7-i)) & 1) == 1) symb_decode = symb_decode->droit;
      else symb_decode = symb_decode->gauche;
      // Si on a atteint une fauille
      if (symb_decode->droit == NULL && symb_decode->gauche == NULL) {
	switch (symb_decode->symb) {
	case (uint8_t) 0x00:
	  resi = 63;
	  break;
	case (uint8_t) 0xf0:
	  resi += 16;
	  break;
	default:
	  uint8_t alpha = symb_decode->symb >> 4;
	  uint8_t gamma = symb_decode->symb & 0b00001111;
	  resi += alpha;
	  if (gamma + i < 8) {
	    uint16_t indice = (c>>(7-i-gamma)) & ((1<<gamma)-1);
	    i += gamma;
	    res[resi] = get_val_from_magnitude(gamma, indice);
	  } else {
	    uint16_t indice = (c & ((1<<(7-i))-1))<<(gamma-(7-i));
	    //c = fgetc(file);
	    c = my_getc(file, c);
	    if (gamma + i < 16) {
	      indice += (c&((1<<8)-1))>>(8-(gamma-(7-i)));
	      i = gamma-(7-i)-1;
	      res[resi] = get_val_from_magnitude(gamma, indice);
	    } else {
	      indice += (c&((1<<8)-1))<<(gamma-(7-i)-8);
	      c = my_getc(file, c);
	      indice += (c&((1<<8)-1))>>(8-(gamma-(7-i)-8));
	      i = (gamma-(7-i)-8)-1;
	      res[resi] = get_val_from_magnitude(gamma, indice);
	    }
	  }
	  resi++;
	  break;
	}
	symb_decode = ht;
	if (resi >= 63) break;
      }
      i++;
    }
    if (resi >= 63) break;
    //c = fgetc(file);
    c = my_getc(file, c);
    i = 0;
  }
  *off = i+1;
  return res;
}
