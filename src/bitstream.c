#include <stdio.h>
#include <stdlib.h>

#include <erreur.h>
#include <bitstream.h>


// Lit un charactère à l'adresse <file> et la place sur <*char>
static erreur_t my_getc(FILE *file, char *c);


static erreur_t my_getc(FILE* file, char *c) {
   if (*c == (char) 0xff) {
      *c = fgetc(file);
      if (*c != (char) 0x00) {
         char *str = malloc(80);
         sprintf(str, "Pas de 0x00 après un 0xff (Pas bien !!) %lx\n", ftell(file)-1);
         return (erreur_t) {.code = ERR_0XFF00, str, .must_free = true};
      }
   }
   *c = fgetc(file);
   return (erreur_t) {.code = SUCCESS};
}

void init_bitstream(bitstream_t *bs, FILE *file) {
   bs->file = file;
   bs->off = 0;
   bs->c = fgetc(file);
}

erreur_t read_bit(bitstream_t *bs, uint8_t *bit) {
   *bit = (((bs->c)>>(7-(bs->off))) & 1);
   bs->off++;
   if (bs->off == 8) {
      bs->off = 0;
      erreur_t err = my_getc(bs->file, &(bs->c));
      if (err.code) return err;
   }
   return (erreur_t) {.code = SUCCESS};
}

erreur_t finir_octet(bitstream_t *bs) {
   if (bs->off == 0) {
      fseek(bs->file, -1, SEEK_CUR);
   } else {
      if (bs->c == (char) 0xff) {
         char c = fgetc(bs->file);
         if (c != 0x00) {
            char *str = malloc(80);
            sprintf(str, "Pas de 0x00 après un 0xff (Pas bien !!) %lx\n", ftell(bs->file)-1);
            return (erreur_t) {.code = ERR_0XFF00, str, .must_free = true};
         }
      }
   }
   return (erreur_t) {.code = SUCCESS};
}
