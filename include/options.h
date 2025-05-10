#pragma once

#include <stdint.h>


struct all_option_s {
  char *execname;
  int verbose;
  int print_time;
  int idct_fast;
  char *filepath;
  char *outfile;
  uint64_t timer;
  uint64_t abs_timer;
};
typedef struct all_option_s all_option_t;

void print_help(all_option_t *all_option);
void set_option(all_option_t *all_option, const int argc, char **argv);
