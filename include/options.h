#pragma once

#include <stdbool.h>

#include <erreur.h>


// Structure contenant les informations sur les différentes options disponibles
struct all_option_s {
   char *execname;
   bool verbose;
   bool  print_time;
   bool idct_fast;
   char *filepath;
   char *outfile;
   bool print_tables;
   bool print_help;
};
typedef struct all_option_s all_option_t;

// Affiche sur la sortie standard une aide pour les options
erreur_t print_help(all_option_t *all_option);

// Renseigne les valeurs des options activées en ligne de commande
erreur_t set_option(all_option_t *all_option, const int argc, char **argv);
