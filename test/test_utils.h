#pragma once

#include <stdbool.h>

// Affiche le résultat du test dont le nom est <format> du fichier <argv[0]> indiqué par le booléen <test_var> (1 pour réussi, 0 pour raté)
void test_res(bool test_var, char *argv[], char *format, ...);
