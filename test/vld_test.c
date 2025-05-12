
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

all_option_t all_option;

static void free_huffman_tree(huffman_tree_t *tree) {
  if (tree == NULL) return;
  free_huffman_tree(tree->droit);
  free_huffman_tree(tree->gauche);
  free(tree);
}

int main(int argc, char **argv) {
  huffman_tree_t *dc = (huffman_tree_t*) calloc(1, sizeof(huffman_tree_t));
  dc->droit = (huffman_tree_t*) calloc(1, sizeof(huffman_tree_t));
  dc->gauche = (huffman_tree_t*) calloc(1, sizeof(huffman_tree_t));
  dc->gauche->symb = 3;
  // 0 -> 3
  // 1 -> Interdit
  
  huffman_tree_t *ac = (huffman_tree_t*) calloc(1, sizeof(huffman_tree_t));
  ac->gauche = (huffman_tree_t*) calloc(1, sizeof(huffman_tree_t));
  ac->gauche->symb = 0x00;
  ac->droit = (huffman_tree_t*) calloc(1, sizeof(huffman_tree_t));
  ac->droit->gauche = (huffman_tree_t*) calloc(1, sizeof(huffman_tree_t));
  ac->droit->gauche->symb = 0xf0;
  ac->droit->droit = (huffman_tree_t*) calloc(1, sizeof(huffman_tree_t));
  ac->droit->droit->gauche = (huffman_tree_t*) calloc(1, sizeof(huffman_tree_t));
  ac->droit->droit->gauche->symb = 0x80;
  ac->droit->droit->droit = (huffman_tree_t*) calloc(1, sizeof(huffman_tree_t));
  ac->droit->droit->droit->gauche = (huffman_tree_t*) calloc(1, sizeof(huffman_tree_t));
  ac->droit->droit->droit->gauche->symb = 0x22;
  // 0 -> 0x00
  // 10 -> 0xf0
  // 110 -> 0x80
  // 1110 -> 0x22
  // 1111 -> Interdit

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

  int nb_test = 5;
  int bsize[] = {1, 1, 2, 2, 1};
  int outsize[] = {2, 0, 5, 20, 0};
  uint8_t *blocs[] = {bloc1, bloc2, bloc3, bloc4, bloc5};
  int16_t *outs[] = {out1, out2, out3, out4, out5};

  for (int test=0; test<nb_test; test++) {
    int fd[2];
    pipe(fd);
    
    FILE *f = fopen("/tmp/vld_test_d", "w+");
    fwrite(blocs[test], sizeof(uint8_t), bsize[test], f);
    fclose(f);
    int pid = fork();
    if (pid == 0) {
      int my_stdout[2];
      pipe(my_stdout);
      dup2(my_stdout[1], STDOUT_FILENO);
      dup2(my_stdout[1], STDERR_FILENO);
      FILE *f = fopen("/tmp/vld_test_d", "r");
      int16_t dc_prec = 0;
      uint8_t off = 0;
      blocl16_t *bl = decode_bloc_acdc(f, dc, ac, &dc_prec, &off);
      int16_t out[64];
      read(fd[0], out, 64*sizeof(int16_t));
      for (size_t i=0; i<64; i++) {
	if (bl->data[i] != out[i]) {
	  exit(2);
	}
      }
      fclose(f);
      close(my_stdout[0]);
      close(my_stdout[1]);
      exit(EXIT_SUCCESS);
    }
    int16_t out[64];
    for (size_t i=0; i<64; i++) {
      if (i<outsize[test]) out[i] = outs[test][i];
      else out[i] = 0;
    }
    write(fd[1], out, 64*sizeof(int16_t));
    int status;
    waitpid(pid, &status, 0);
    if ((WEXITSTATUS(status) == EXIT_FAILURE && outsize[test] != 0) || WEXITSTATUS(status) == 2) {
      char str[8];
      sprintf(str, "Test %d", test+1);
      test_res(false, str, argv);
    }
    close(fd[0]);
    close(fd[1]);
  }

  free_huffman_tree(ac);
  free_huffman_tree(dc);
  
  return 0;
}
