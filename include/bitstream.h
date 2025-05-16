#pragma once

#include <stdio.h>
#include <stdint.h>

#include <erreur.h>

struct bitstream_s {
   FILE *file;
   uint8_t off;
   char c;
};
typedef struct bitstream_s bitstream_t;

void init_bitstream(bitstream_t *bs, FILE *file);
erreur_t read_bit(bitstream_t *bs, uint8_t *bit);
erreur_t finir_octet(bitstream_t *bs);
