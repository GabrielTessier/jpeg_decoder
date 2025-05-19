#pragma once

#include <stdint.h>
#include <stdbool.h>


// Structure pour mesurer le temps écoulé durant l'exécution.
struct my_timer_s {
   bool state;
   uint64_t init;
   uint64_t sum;
};
typedef struct my_timer_s my_timer_t;

// Initialise le chronomètre.
void init_timer(my_timer_t *timer);

// Démarre le chronomètre.
void start_timer(my_timer_t *timer);

// Arrête le chronomètre.
void stop_timer(my_timer_t *timer);

// Affiche la durée mesurée (stockée dans <*timer>) sur la sortie standard après la chaine de charactères <text>.
void print_timer(char* text, my_timer_t *timer);
