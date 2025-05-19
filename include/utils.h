#pragma once

#include <stdio.h>

#include <erreur.h>
#include <img.h>


// Fonction d'affichage sur la sortie standard conditionnée à l'option `verbose`.
void print_v(const char *format, ...);

// Affiche la table de huffman <*tree> si option `verbose` est vérifiée.
void print_hufftable(huffman_tree_t *tree);

// Ouvre le fichier <all_option.filepath> et renseigne <*fichier> avec le pointeur obtenu.
// Vérifie l'extension et la bonne ouverture du fichier.
erreur_t ouverture_fichier_in(FILE **fichier);

/* Retourne le nom du fichier de sortie.
 * nbcomp : nombre de composantes, pour déterminer l'extension : .ppm pour la couleur, .pgm pour le N&B
 * nb : l'indice de l'image générée (pour le cas progressif, où plusieurs images sont générées) */
char *out_file_name(uint8_t nbcomp, uint8_t nb);

// Ouvre le fichier de sortie et retourne un pointeur vers celui-ci.
// <nbcomp> et <nb> sont nécessaires pour l'appel à `out_file_name`.
FILE *ouverture_fichier_out(uint8_t nbcomp, uint8_t nb);
