#pragma once

#include <stdio.h>
#include <stdint.h>

#include <erreur.h>


// Structure pour stocker les bits en cours de lecture
struct bitstream_s {
   FILE *file; 	 // fichier en cours de lecture
   uint8_t off;  // offset par rapport à l'octet pointé par <file>
   char c;	 // octet actuel
};
typedef struct bitstream_s bitstream_t;

// Initialise le bitstream <bs> sur le pointeur <file>
void init_bitstream(bitstream_t *bs, FILE *file);

// Lit un bit à l'adresse <bs->file> et le place dans <*bit>
erreur_t read_bit(bitstream_t *bs, uint8_t *bit);

// Lit le reste de l'octet en cours dans <*bs>
erreur_t finir_octet(bitstream_t *bs);
