#pragma once

#include <sys/time.h>
#include <stdint.h>
#include <stdbool.h>

struct my_timer_s {
   bool state;
   uint64_t init;
   uint64_t sum;
};
typedef struct my_timer_s my_timer_t;

void init_timer(my_timer_t *timer);
void start_timer(my_timer_t *timer);
void stop_timer(my_timer_t *timer);
void print_timer(char* text, my_timer_t *timer);
