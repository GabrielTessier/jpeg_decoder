
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>

#include <vld.h>
#include <entete.h>
#include <options.h>
#include <bitstream.h>
#include "erreur.h"
#include "jpeg2ppm.h"
#include "test_utils.h"

static void mon_free_huffman_tree(huffman_tree_t *tree) {
   if (tree == NULL) return;
   mon_free_huffman_tree(tree->fils[1]);
   mon_free_huffman_tree(tree->fils[0]);
   free(tree);
}

int main(int argc, char **argv) {
   (void) argc; // Pour ne pas avoir un warnning unused variable
  
   huffman_tree_t *dc = (huffman_tree_t*) calloc(1, sizeof(huffman_tree_t));
   dc->fils[1] = (huffman_tree_t*) calloc(1, sizeof(huffman_tree_t));
   dc->fils[0] = (huffman_tree_t*) calloc(1, sizeof(huffman_tree_t));
   dc->fils[0]->symb = 3;
   // 0 -> 3
   // 1 -> Interdit
  
   huffman_tree_t *ac = (huffman_tree_t*) calloc(1, sizeof(huffman_tree_t));
   ac->fils[0] = (huffman_tree_t*) calloc(1, sizeof(huffman_tree_t));
   ac->fils[0]->symb = 0x00;
   ac->fils[1] = (huffman_tree_t*) calloc(1, sizeof(huffman_tree_t));
   ac->fils[1]->fils[0] = (huffman_tree_t*) calloc(1, sizeof(huffman_tree_t));
   ac->fils[1]->fils[0]->symb = 0xf0;
   ac->fils[1]->fils[1] = (huffman_tree_t*) calloc(1, sizeof(huffman_tree_t));
   ac->fils[1]->fils[1]->fils[0] = (huffman_tree_t*) calloc(1, sizeof(huffman_tree_t));
   ac->fils[1]->fils[1]->fils[0]->symb = 0x80;
   ac->fils[1]->fils[1]->fils[1] = (huffman_tree_t*) calloc(1, sizeof(huffman_tree_t));
   ac->fils[1]->fils[1]->fils[1]->fils[0] = (huffman_tree_t*) calloc(1, sizeof(huffman_tree_t));
   ac->fils[1]->fils[1]->fils[1]->fils[0]->symb = 0x22;
   ac->fils[1]->fils[1]->fils[1]->fils[1] = (huffman_tree_t*) calloc(1, sizeof(huffman_tree_t));
   ac->fils[1]->fils[1]->fils[1]->fils[1]->fils[0] = (huffman_tree_t*) calloc(1, sizeof(huffman_tree_t));
   ac->fils[1]->fils[1]->fils[1]->fils[1]->fils[0]->symb = 0x3a;
   // 0	        -> 0x00
   // 10	-> 0xf0
   // 110	-> 0x80 (interdit en baseline)
   // 1110	-> 0x22
   // 11110	-> 0x3b
   // 11111	-> Interdit

   // DC = 6
   // AC = 0 ...
   uint8_t bloc1[] = {0b01100000};  // DC : 0(3) 110(6) // AC : 0(EOB)
   int16_t out1[] = {6, 0};

   // DC = Interdit (code avec que 1)
   // AC = 0...
   uint8_t bloc2[] = {0b10000000};  // DC : symbole interdit
   int16_t out2[] = {};
  
   // DC = 6
   // AC = 003 0...
   uint8_t bloc3[] = {0b01101110, 0b11000000};  // DC : 0(3) 110(6) // AC : 1110(0x22) 11(3) 0(EOB)
   int16_t out3[] = {6, 0, 0, 3, 0};
  
   // DC = 6
   // AC = 0000000000000000 002 0...
   uint8_t bloc4[] = {0b01101011, 0b10100000};  // DC : 0(3) 110(6) // AC : 10(0xf0) 1110(0x22) 10(2) 0(EOB)
   int16_t out4[] = {6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0};

   // DC = 6
   // AC = Invalide
   uint8_t bloc5[] = {0b01101100};  // DC : 0(3) 110(6) // AC : 110(0x80) interdit
   int16_t out5[] = {};

   // DC = 6
   // AC = 000 1026 0...
   uint8_t bloc6[] = {0b01101111, 0b01000000, 0b00100000};  // DC : 0(3) 110(6) // AC : 11110(0x3a) 1000000001(513) 0(EOB)
   int16_t out6[] = {6, 0, 0, 0, 513, 0};

   int nb_test = 6;
   int bsize[] = {1, 1, 2, 2, 1, 3};
   int outsize[] = {2, 0, 5, 20, 0, 6};
   erreur_code_t errcode[] = {SUCCESS, ERR_HUFF_CODE_1, SUCCESS, SUCCESS, ERR_AC_BAD, SUCCESS};
   uint8_t *blocs[] = {bloc1, bloc2, bloc3, bloc4, bloc5, bloc6};
   int16_t *outs[] = {out1, out2, out3, out4, out5, out6};
   char *name[] = {"Test DC normal", "Test DC symbole interdit", "Test AC 0xalpha gamma", "Test AC 0xF0", "Test AC 0x?0", "Test avec magnitude supérieure à 8"};
   
   for (int test=0; test<nb_test; test++) {
      // On stocke le bloc dans un fichier temporaire pour le lire dans le décodage des blocs
      FILE *f = fopen("/tmp/vld_test_d", "w");
      fwrite(blocs[test], sizeof(uint8_t), bsize[test], f);
      fclose(f);

      f = fopen("/tmp/vld_test_d", "r");
      bitstream_t *bs = (bitstream_t*) malloc(sizeof(bitstream_t));
      init_bitstream(bs, f);

      img_t *img = (img_t*) malloc(sizeof(img_t));
      img->other = calloc(1, sizeof(other_t));
      img->other->ss = 0;
      img->other->se = 63;
      img->other->ah = 0;
      img->other->al = 0;
      img->section = calloc(1, sizeof(section_done_t));
      img->section->num_sof = 0;
      
      int16_t dc_prec = 0;
      blocl16_t *bl = calloc(1, sizeof(blocl16_t));
      for (int i=0; i<64; i++) bl->data[i] = 0;
      uint16_t skip_bloc;
      erreur_t err = decode_bloc_acdc(bs, img, dc, ac, bl, &dc_prec, &skip_bloc);
      if (err.code != errcode[test]) {
	 printf("%s\n", err.com);
	 test_res(false, argv, "%s (Code de retour invalide : reçu : %d, attendu : %d)", name[test], err.code, errcode[test]);
      } else {
	 bool passed = true;
	 if (err.code == SUCCESS) {
	    for (int i=0; i<64; i++) {
	       int16_t out = (i<outsize[test]) ? outs[test][i] : 0;
	       if (bl->data[i] != out) {
		  printf("diff\n");
		  passed = false;
	       }
	    }
	 }
	 test_res(passed, argv, "%s (%d)", name[test], err.code);
      }
      if (err.must_free) free(err.com);
      free(img->section);
      free(img->other);
      free(img);  
      free(bl);
      free(bs);
      fclose(f);
   }

   mon_free_huffman_tree(ac);
   mon_free_huffman_tree(dc);
  
   return 0;
}
