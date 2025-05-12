#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/time.h>
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
  if (tree->droit == NULL && tree->gauche == NULL) {
    print_v("path : %s symbol : %x\n", acu, tree->symb);
    return;
  }
  int i = strlen(acu);
  acu[i] = '0';
  print_hufftable_acu(acu, tree->gauche);
  acu[i] = '1';
  print_hufftable_acu(acu, tree->droit);
  acu[i] = 0;
}

void print_hufftable(huffman_tree_t* tree) {
  if (all_option.verbose) {
    char acu[30];
    print_hufftable_acu(acu, tree);
  }
}

uint64_t cast_time(struct timeval time) {
  return time.tv_sec*1000000 + time.tv_usec;
}

void init_timer() {
  if (all_option.print_time) {
    struct timeval t;
    gettimeofday(&t, NULL);
    all_option.timer = cast_time(t);
    all_option.abs_timer = all_option.timer;
  }
}

void start_timer() {
  if (all_option.print_time) {
    struct timeval t;
    gettimeofday(&t, NULL);
    all_option.timer = cast_time(t);
  }
}

void print_timer(char* text) {
  if (all_option.print_time) {
    struct timeval t;
    gettimeofday(&t, NULL);
    uint64_t tt = cast_time(t);
    fprintf(stdout, "%s : %f s\n", text, (float) (tt-all_option.timer)/1000000);
    all_option.timer = tt;
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
