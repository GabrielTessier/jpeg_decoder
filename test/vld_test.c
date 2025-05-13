
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>

#include <vld.h>
#include <options.h>
#include "test_utils.h"

static void free_huffman_tree(huffman_tree_t *tree) {
   if (tree == NULL) return;
   free_huffman_tree(tree->fils[1]);
   free_huffman_tree(tree->fils[0]);
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
   ac->fils[1]->fils[1]->fils[1]->fils[1]->fils[0]->symb = 0x3b;
   // 0	-> 0x00
   // 10	-> 0xf0
   // 110	-> 0x80
   // 1110	-> 0x22
   // 11110	-> 0x3b
   // 11111	-> Interdit

   // DC = 6
   // AC = 0 ...
   uint8_t bloc1[] = {0b01100000};
   int16_t out1[] = {6, 0};

   // DC = Interdit (code avec que 1)
   // AC = 0...
   uint8_t bloc2[] = {0b10000000};
   int16_t out2[] = {};
  
   // DC = 6
   // AC = 003 0...
   uint8_t bloc3[] = {0b01101110, 0b11000000};
   int16_t out3[] = {6, 0, 0, 3, 0};
  
   // DC = 6
   // AC = 0000000000000000 002 0...
   uint8_t bloc4[] = {0b01101011, 0b10100000};
   int16_t out4[] = {6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0};

   // DC = 6
   // AC = Invalide
   uint8_t bloc5[] = {0b01101100};
   int16_t out5[] = {};

   // DC = 6
   // AC = 000 1026 0...
   uint8_t bloc6[] = {0b01101111, 0b01000000, 0b00100000};
   int16_t out6[] = {6, 0, 0, 0, 1026, 0};

   int nb_test = 6;
   int bsize[] = {1, 1, 2, 2, 1, 3};
   int outsize[] = {2, 0, 5, 20, 0, 6};
   uint8_t *blocs[] = {bloc1, bloc2, bloc3, bloc4, bloc5, bloc6};
   int16_t *outs[] = {out1, out2, out3, out4, out5, out6};
   char *name[] = {"Test DC normal", "Test DC symbole interdit", "Test AC 0xalpha gamma", "Test AC 0xF0", "Test AC 0x?0", "Test avec magnitude supérieure à 8"};

   for (int test=0; test<nb_test; test++) {
      int fd_out[2];	       	// Pour envoyer le tableau out au process enfant
      pipe(fd_out);
      int my_stdout[2];		// Pour récupérer le sortie (standard et d'erreur) du process enfant
      pipe(my_stdout);

      // On stocke le bloc dans un fichier temporaire pour le lire dans le décodage des blocs
      FILE *f = fopen("/tmp/vld_test_d", "w+");
      fwrite(blocs[test], sizeof(uint8_t), bsize[test], f);
      fclose(f);
      // On décode le bloc dans un process enfant à cause des exit(EXIT_FAILURE)
      int pid = fork();
      if (pid == 0) {
	 // On redirige stdour et stderr vers le pipe my_stdout
	 dup2(my_stdout[1], STDOUT_FILENO);
	 dup2(my_stdout[1], STDERR_FILENO);
      
	 FILE *f = fopen("/tmp/vld_test_d", "r");
	 int16_t dc_prec = 0;
	 uint8_t off = 0;
	 blocl16_t *bl = decode_bloc_acdc(f, dc, ac, &dc_prec, &off);
	 int16_t out[64];
	 read(fd_out[0], out, 64*sizeof(int16_t));
	 for (int i=0; i<64; i++) {
	    if (bl->data[i] != out[i]) {
	       exit(2);  // Code d'erreur 2 dans le cas où le bloc décodé n'est pas bon
	    }
	 }
	 fclose(f);
	 close(my_stdout[0]);
	 close(my_stdout[1]);
	 exit(EXIT_SUCCESS);
      }
      int16_t out[64];
      for (int i=0; i<64; i++) {
	 if (i<outsize[test]) out[i] = outs[test][i];
	 else out[i] = 0;
      }
      write(fd_out[1], out, 64*sizeof(int16_t));
      int status;
      waitpid(pid, &status, 0);
      test_res(!((WEXITSTATUS(status) == EXIT_FAILURE && outsize[test] != 0) || WEXITSTATUS(status) == 2), argv, name[test]);
      close(fd_out[0]);
      close(fd_out[1]);
   }

   free_huffman_tree(ac);
   free_huffman_tree(dc);
  
   return 0;
}
