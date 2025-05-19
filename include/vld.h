#pragma once

#include <stdint.h>

#include <jpeg2ppm.h>
#include <img.h>
#include <bitstream.h>
#include <erreur.h>

// Enumération pour indiquer le type de traitement voulu
enum acdc_e { AC, DC };

/* Décode un bloc de DC/AC
 * hdc : table de huffman pour les DC
 * hac : table de huffman pour les AC
 * sortie : bloc en cours de traitement
 * dc_prec : pointeur vers le DC précédent
 * skip_bloc : pointeur vers la variable contenant le nombre de bloc à sauter
 */
erreur_t decode_bloc_acdc(bitstream_t *bs, img_t *img, huffman_tree_t *hdc, huffman_tree_t *hac, blocl16_t *sortie, int16_t *dc_prec, uint16_t *skip_bloc);

// Corrige les coefficients non nuls de la bande spectrale
erreur_t correction_eob(bitstream_t *bs, img_t *img, blocl16_t *sortie, uint64_t *resi);
