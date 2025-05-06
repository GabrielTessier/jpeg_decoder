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

int8_t *decodeDC(huffman_tree_t* ht, FILE* file, uint64_t pos, uint64_t size) {
  int8_t diff = 0;
  fseek(file, pos, SEEK_SET);// On se place au début du code
  int8_t* res = (int8_t*) malloc(sizeof(int8_t)*size); // tableau contenant les dc décodé
  uint64_t resi = 0; // indice de où on en est dans res
  huffman_tree_t* symb_decode = ht; // où on en est dans l'arbre de huffman
  // On lit char par char mais on traite bit par bit
  while (true) {
    char c = fgetc(file);
    int i=0;
    while (i<8) {
      // on regarde si le bit courant est 0 ou 1
      if (((c>>(8-i)) & 1) == 1) symb_decode = symb_decode->droit;
      else symb_decode = symb_decode->gauche;
      // Si on a atteint une fauille
      if (symb_decode->droit == NULL && symb_decode->gauche == NULL) {
	// Si la valeur à lire est entièrement sur le char courant
	if (symb_decode->symb + i < 8) {
	  // On récupère la suite de bit sous forme de int puis on incrémente i
	  uint16_t indice = (c>>(8-i-symb_decode->symb)) & ((1<<symb_decode->symb)-1);
	  res[resi] = diff + get_val_from_magnitude(symb_decode->symb, indice);
	} else {
	  // Si la valeur à lire est sur deux char
	  // On lit les bits de poids fort puis ceux de poids faible
	  uint16_t indice = (c & (1<<(7-i)))<<(symb_decode->symb-(8-i));
	  c = fgetc(file);
	  indice += c>>(8-(symb_decode->symb-(8-i)));
	  i = symb_decode->symb-(8-i);
	  res[resi] = diff + get_val_from_magnitude(symb_decode->symb, indice);
	}
	diff = res[resi];
	resi++;
	symb_decode = ht;
	if (resi == size) break;
      }
      i++;
    }
    if (resi == size) break;
  }
  return res;
}

int8_t *decodeAC(huffman_tree_t* ht, FILE* file, uint64_t pos) {

  fseek(file, pos, SEEK_SET);
  int8_t *res = (int8_t*) calloc(63, sizeof(int8_t));
  uint8_t resi = 0;
  
  huffman_tree_t* symb_decode = ht;
  while (true) {
    char c = fgetc(file);
    int i=0;
    while (i<8) {
      // on regarde si le bit courant est 0 ou 1
      if (((c>>(8-i)) & 1) == 1) symb_decode = symb_decode->droit;
      else symb_decode = symb_decode->gauche;
      // Si on a atteint une fauille
      if (symb_decode->droit == NULL && symb_decode->gauche == NULL) {
	switch (symb_decode->symb) {
	case (char) 0x00:
	  return res;
	case (char) 0xF0:
	  resi += 16;
	  break;
	default:
	  uint8_t alpha = symb_decode->symb >> 4;
	  uint8_t gamma = symb_decode->symb & 0b00001111;
	  resi += alpha;
	  if (gamma + i < 8) {
	    uint16_t indice = (c>>(8-i-gamma)) & ((1<<gamma)-1);
	    res[resi] = get_val_from_magnitude(gamma, indice);
	  } else {
	    uint16_t indice = (c & (1<<(7-i)))<<(gamma-(8-i));
	    c = fgetc(file);
	    indice += c>>(8-(gamma-(8-i)));
	    i = gamma-(8-i);
	    res[resi] = get_val_from_magnitude(gamma, indice);
	  }
	  resi++;
	  break;
	}
	symb_decode = ht;
	if (resi == 63) break;
      }
      i++;
    }
    if (resi == 63) break;
  }
  return res;
}
