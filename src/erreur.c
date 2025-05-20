#include <stdio.h>
#include <stdlib.h>

#include <colors.h>
#include <erreur.h>

void print_erreur(const erreur_t err) {
   fprintf(stderr, BOLDRED "ERREUR %d" RESET " : %s\n", err.code, err.com);
   if (err.must_free) free(err.com);
}
