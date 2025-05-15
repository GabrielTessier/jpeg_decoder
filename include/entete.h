#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include <img.h>
#include <erreur.h>

// Décode et renvoie les informations de l'entête de l'image
erreur_t decode_entete(FILE *fichier, bool premier_passage, img_t *img);
