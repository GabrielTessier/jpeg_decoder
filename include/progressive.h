#pragma once

#include <stdio.h>
#include <img.h>
#include <erreur.h>

// Décode l'image en mode progressif à partir de <*infile> et de <*img>
erreur_t decode_progressive_image(FILE *infile, img_t *img);
