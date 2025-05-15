#pragma once

#include <stdio.h>
#include <entete.h>

void print_huffman_quant_table(img_t *img);

int16_t get_composante(img_t *img, uint8_t k);

void save_mcu_ligne_bw(FILE *outputfile, img_t *img, bloctu8_t ***ycc);
void save_mcu_ligne_color(FILE *outputfile, img_t *img, bloctu8_t ***ycc, char *rgb, uint8_t yhf, uint8_t yvf, uint8_t y_id, uint64_t nb_blocYH, uint8_t cbhf, uint8_t cbvf, uint8_t cb_id, uint64_t nb_blocCbH, uint8_t crhf, uint8_t crvf, uint8_t cr_id, uint64_t nb_blocCrH);
