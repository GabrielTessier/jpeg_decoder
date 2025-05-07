#pragma once

#include <stdint.h>

extern int verbose;
extern int print_time;
extern char *filepath;
extern char *outfile;
extern uint64_t timer;
extern uint64_t abs_timer;


void set_option(const int argc, char **argv);
