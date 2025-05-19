#pragma once

#include <stdio.h>
#include <stdbool.h>

#include <img.h>
#include <erreur.h>


// Décode les informations de l'entête de l'image jusqu'au SOS suivant.
// Lit à partir de <fichier> et stocke les informations de l'entête dans <*img>.
// Le comportement de la fonction varie si c'est sa première lecture de <fichier>, indiqué par <premier_passage>.
erreur_t decode_entete(FILE *fichier, bool premier_passage, img_t *img);
