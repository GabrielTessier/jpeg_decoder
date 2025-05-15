#include <stdio.h>

#include <colors.h>
#include <erreur.h>

void print_erreur(const erreur_t err) {
  fprintf(stderr, RED "ERREUR %d" RESET " : %s\n", err.code, err.com);
}
