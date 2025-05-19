#include <stdio.h>
#include <sys/time.h>

#include <options.h>
#include <timer.h>


extern all_option_t all_option;

static uint64_t cast_time(struct timeval time) {
   return time.tv_sec*1000000 + time.tv_usec;
}

void init_timer(my_timer_t *timer) {
   timer->init = 0;
   timer->state = false;
   timer->sum = 0;
}

void start_timer(my_timer_t *timer) {
   struct timeval t;
   gettimeofday(&t, NULL);
   timer->init = cast_time(t);
   timer->state = true;
}

void stop_timer(my_timer_t *timer) {
   if (timer->state) {
      struct timeval t;
      gettimeofday(&t, NULL);
      timer->sum += cast_time(t) - timer->init;
      timer->state = false;
   }
}

void print_timer(char* text, my_timer_t *timer) {
   if (all_option.print_time) {
      struct timeval t;
      gettimeofday(&t, NULL);
      uint64_t tt = cast_time(t);
      uint64_t total = timer->sum;
      if (timer->state) total += tt-timer->init;
      fprintf(stdout, "%s : %f s\n", text, (float) total/1000000);
   }
}
