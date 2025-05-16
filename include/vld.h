#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <jpeg2ppm.h>
#include <img.h>
#include <bitstream.h>
#include <erreur.h>

// Enumération pour indiquer le type de traitement voulu
enum acdc_e { AC, DC };

// Retourne le bloc (AC et DC) commençant à <fichier>+<off>,
// décodé à l'aide des tables de Huffman <hdc> (DC) et <hac> (AC).
// On retrouve la composante constante de ce bloc fréquentiel
// à l'aide de <dc_prec>.
erreur_t decode_bloc_acdc(bitstream_t *bs, img_t *img, huffman_tree_t *hdc, huffman_tree_t *hac, blocl16_t *sortie, int16_t *dc_prec, uint16_t *skip_bloc);

erreur_t correction_eob(bitstream_t *bs, img_t *img, blocl16_t *sortie, uint64_t *resi);
