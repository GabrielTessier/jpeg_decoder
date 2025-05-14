#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include <vld.h>
#include <options.h>
#include <utils.h>


extern all_option_t all_option;

void print_v(const char* format, ...) {
  if (all_option.verbose) {
    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);
  }
}

static void print_hufftable_acu(char* acu, huffman_tree_t* tree) {
  if (tree->fils[0] == NULL && tree->fils[1] == NULL) {
    print_v("path : %s symbol : %x\n", acu, tree->symb);
    return;
  }
  int i = strlen(acu);
  acu[i] = '0';
  print_hufftable_acu(acu, tree->fils[0]);
  acu[i] = '1';
  print_hufftable_acu(acu, tree->fils[1]);
  acu[i] = 0;
}

void print_hufftable(huffman_tree_t* tree) {
  if (all_option.verbose) {
    char acu[30];
    print_hufftable_acu(acu, tree);
  }
}

void erreur(const char* text, ...) {
    fprintf(stderr, "ERREUR : ");
    va_list args;
    va_start(args, text);
    vfprintf(stderr, text, args);
    va_end(args);
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}
