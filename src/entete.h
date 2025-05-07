#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "iqzz.h"
#include "vld.h"


struct qtable_prec_s {
    uint8_t precision;
    qtable_t *qtable;
};
typedef struct qtable_prec_s qtable_prec_t;

struct htables_s {
    huffman_tree_t **dc;
    huffman_tree_t **ac;
};
typedef struct htables_s htables_t;

struct idcomp_s {
    uint8_t idc;
    uint8_t hsampling;
    uint8_t vsampling;
    uint8_t idhdc;
    uint8_t idhac;
    uint8_t idq;
};
typedef struct idcomp_s idcomp_t;

struct comps_s {
    uint8_t nb;
    uint8_t ordre[3];
    uint8_t precision_comp;
    idcomp_t **comps;
};
typedef struct comps_s comps_t;

struct other_s {
    char jfif[5];
    uint8_t version_jfif_a;
    uint8_t version_jfif_b;
    uint8_t ss;
    uint8_t se;
    uint8_t ah;
    uint8_t al;
};
typedef struct other_s other_t;

struct img_s {
    uint16_t height;
    uint16_t width;
    qtable_prec_t **qtables;
    htables_t *htables;
    comps_t *comps;
    bool sosdone;
    other_t *other;
};
typedef struct img_s img_t;

struct couple_tree_depth_s {
    huffman_tree_t *tree;
    uint8_t depth;
};
typedef struct couple_tree_depth_s couple_tree_depth_t;


img_t* decode_entete(FILE *fichier);

void erreur(const char* text, ...);

void marqueur(FILE *fichier, img_t *img);

void app0(FILE *fichier, img_t *img);

void com(FILE *fichier);

void dqt(FILE *fichier, img_t *img);

void sof0(FILE *fichier, img_t *img);

void dht(FILE *fichier, img_t *img);

void sos(FILE *fichier, img_t *img);
