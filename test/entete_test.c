#include <utils.h>
#include <entete.h>
#include <options.h>
#include "test_utils.h"

all_option_t all_option;


int main(int argc, char *argv[]) {
    FILE *fichier = fopen("./test/test_file/invader.jpeg", "r");
    img_t *img = decode_entete(fichier);

    int test_taille = 1;
    int test_qtables = 1;
    int test_htables = 1;
    int test_comps = 1;
    int test_other = 1;
    int test_sampling = 1;
    int test_mcu = 1;

    // TAILLE
    if (img->height != 8) test_taille = 0;
    if (img->width != 8) test_taille = 0;
    test_res(test_taille, argv, "décode taille");

    // QTABLES
    if (img->qtables[0]->precision != 0) test_qtables = 0;
    qtable_t *qtable = img->qtables[0]->qtable;
    uint8_t ref[64] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
    for (int i=0; i<64; i++) {
        if (qtable->data[i] != ref[i]) test_qtables = 0;
    }
    for (int i=1; i<4; i++) {
        if (img->qtables[i] != NULL) test_qtables = 0;
    }
    test_res(test_qtables, argv, "décode qtables");

    // HTABLES
    // DC
    if (img->htables->dc[0]->fils[0]->symb != 7) test_htables = 0;
    for (int i=1; i<4; i++) {
        if (img->htables->dc[i] != NULL) test_htables = 0;
    }
    // AC
    huffman_tree_t *tree = img->htables->ac[0];
    if (tree->fils[0]->fils[0]->symb != 0x17) test_htables = 0;
    if (tree->fils[0]->fils[1]->symb != 0x18) test_htables = 0;
    if (tree->fils[1]->fils[0]->fils[0]->symb != 0x15) test_htables = 0;
    if (tree->fils[1]->fils[0]->fils[1]->fils[0]->symb != 0x8) test_htables = 0;
    if (tree->fils[1]->fils[0]->fils[1]->fils[1]->symb != 0x19) test_htables = 0;
    if (tree->fils[1]->fils[1]->fils[0]->fils[0]->fils[0]->symb != 0x0) test_htables = 0;
    if (tree->fils[1]->fils[1]->fils[0]->fils[0]->fils[1]->symb != 0x9) test_htables = 0;
    if (tree->fils[1]->fils[1]->fils[0]->fils[1]->fils[0]->symb != 0x13) test_htables = 0;
    if (tree->fils[1]->fils[1]->fils[0]->fils[1]->fils[1]->symb != 0x23) test_htables = 0;
    if (tree->fils[1]->fils[1]->fils[1]->fils[0]->fils[0]->symb != 0x28) test_htables = 0;
    if (tree->fils[1]->fils[1]->fils[1]->fils[0]->fils[1]->symb != 0x29) test_htables = 0;
    if (tree->fils[1]->fils[1]->fils[1]->fils[1]->fils[0]->symb != 0x37) test_htables = 0;
    for (int i=1; i<4; i++) {
        if (img->htables->ac[i] != NULL) test_htables = 0;
    }
    test_res(test_htables, argv, "décode htables");

    // COMPS
    if (img->comps->nb != 1) test_comps = 0;
    if (img->comps->ordre[0] != 1) test_comps = 0;
    if (img->comps->ordre[1] != 0) test_comps = 0;
    if (img->comps->ordre[2] != 0) test_comps = 0;
    if (img->comps->precision_comp != 8) test_comps = 0;
    if (img->comps->comps[0]->idc != 1) test_comps = 0;
    if (img->comps->comps[0]->hsampling != 1) test_comps = 0;
    if (img->comps->comps[0]->vsampling != 1) test_comps = 0;
    if (img->comps->comps[0]->idhdc != 0) test_comps = 0;
    if (img->comps->comps[0]->idhac != 0) test_comps = 0;
    if (img->comps->comps[0]->idq != 0) test_comps = 0;
    if (img->comps->comps[1] != NULL) test_comps = 0;
    if (img->comps->comps[2] != NULL) test_comps = 0;
    test_res(test_comps, argv, "décode comps");

    // OTHER
    if (strcmp(img->other->jfif,"JFIF") != 0) test_other = 0;
    if (img->other->version_jfif_x != 1) test_other = 0;
    if (img->other->version_jfif_y != 1) test_other = 0;
    if (img->other->ss != 0) test_other = 0;
    if (img->other->se != 63) test_other = 0;
    if (img->other->ah != 0) test_other = 0;
    if (img->other->al != 0) test_other = 0;
    test_res(test_other, argv, "décode other");

    // SAMPLING
    if (img->max_hsampling != 1) test_sampling = 0;
    if (img->max_vsampling != 1) test_sampling = 0;
    test_res(test_sampling, argv, "décode sampling max");

    // MCU
    if (img->nbmcuH != 1) test_mcu = 0;
    if (img->nbmcuV != 1) test_mcu = 0;
    if (img->nbMCU != 1) test_mcu = 0;
    test_res(test_mcu, argv, "décode nb mcu");

    free_img(img);
    return 0;
}
