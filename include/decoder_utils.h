#pragma once

#include <stdio.h>
#include <erreur.h>
#include <img.h>
#include <jpeg2ppm.h>

// Affiche sur la sortie standard les tables de quantification et les tables de Huffman définies dans <*img>
void print_huffman_quant_table(img_t *img);

// Retourne l'indice de la composante <k> dans <*img> selon l'ordre défini par l'entête
// Retourne -1 si la composante n'est pas renseignée dans <*img>
int16_t get_composante(img_t *img, uint8_t k);

// Écrit à l'adresse <outputfile> la ligne de MCU de pixels en noir et blanc contenue dans <ycc>
erreur_t save_mcu_ligne_bw(FILE *outputfile, img_t *img, bloctu8_t ***ycc);

/* Écrit à l'adresse <outputfile> la ligne de MCU de pixels en couleur (RGB) contenue dans <ycc>
* rgb : tableau pour stocker les composantes RGB de la ligne
* yhf : facteur horizontal de Y
* yvf : facteur vertical de Y
* y_id : indice de la composante Y dans <*img>
* nb_blocYH : nombre de blocs de Y horizontalement
* cbhf : facteur horizontal de Cb
* cbvf : facteur vertical de Cb
* cb_id : indice de la composante Cb dans <*img>
* nb_blocCbH : nombre de blocs de Cb horizontalement
* crhf : facteur horizontal de Cr
* crvf : facteur vertical de Cr
* cr_id : indice de la composante Cr dans <*img>
* nb_blocCrH : nombre de blocs de Cr horizontalement */
erreur_t save_mcu_ligne_color(FILE *outputfile, img_t *img, bloctu8_t ***ycc, char *rgb, uint8_t yhf, uint8_t yvf, uint8_t y_id, uint64_t nb_blocYH, uint8_t cbhf, uint8_t cbvf, uint8_t cb_id, uint64_t nb_blocCbH, uint8_t crhf, uint8_t crvf, uint8_t cr_id, uint64_t nb_blocCrH);

// Calcule plusieurs informations sur les couleurs dans le cas de 3 composantes
void get_ycc_info(img_t *img, uint8_t *y_id, uint8_t *cb_id, uint8_t *cr_id, uint8_t *yhf, uint8_t *yvf, uint8_t *cbhf, uint8_t *cbvf, uint8_t *crhf, uint8_t *crvf, uint64_t *nb_blocYH, uint64_t *nb_blocCbH, uint64_t *nb_blocCrH, char **rgb);
