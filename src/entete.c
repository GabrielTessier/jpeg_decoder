#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include "entete.h"
#include "list.h"
#include "vld.h"


void free_qtables(qtable_prec_t **qtables) {
  for (int i=0; i<4; i++) {
    if (qtables[i] != NULL) {
      free(qtables[i]->qtable);
      free(qtables[i]);
    }
  }
  free(qtables);
}

void free_huffman_tree(huffman_tree_t *tree) {
  if (tree == NULL) return;
  free_huffman_tree(tree->droit);
  free_huffman_tree(tree->gauche);
  free(tree);
}

void free_htables(htables_t *htables) {
  for (int i=0; i<4; i++) {
    if (htables->ac[i] != NULL) free_huffman_tree(htables->ac[i]);
    if (htables->dc[i] != NULL) free_huffman_tree(htables->dc[i]);
  }
  free(htables->ac);
  free(htables->dc);
  free(htables);
}

void free_comps(comps_t *comps) {
  free(comps->comps[0]);
  free(comps->comps[1]);
  free(comps->comps[2]);
  free(comps->comps);
  free(comps);
}

void free_other(other_t *other) {
  free(other);
}

void free_img(img_t *img) {
  free_qtables(img->qtables);
  free_htables(img->htables);
  free_comps(img->comps);
  free_other(img->other);
  free(img);
}

img_t* decode_entete(FILE *fichier) {
    img_t *img = calloc(1,sizeof(img_t));
    img->qtables = calloc(4,sizeof(qtable_prec_t*));
    img->htables = calloc(1,sizeof(htables_t));
    img->htables->dc = calloc(4,sizeof(huffman_tree_t*));
    img->htables->ac = calloc(4,sizeof(huffman_tree_t*));
    img->comps = calloc(1,sizeof(comps_t));
    img->comps->comps = calloc(3,sizeof(idcomp_t*));
    img->other = calloc(1,sizeof(other_t));
    
    while (!img->sosdone) {
        marqueur(fichier, img);
    }
    return img;
}


void erreur(const char* text, ...) {
    va_list args;
    va_start(args, text);
    vfprintf(stderr, text, args);
    va_end(args);
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}


void marqueur(FILE *fichier, img_t *img) {
    char c[2];
    fread(&c, 1, 2, fichier);
    if (c[0] != (char) 0xff) erreur("Ce n'est pas un marqueur : octet n° %ld", ftell(fichier));
    switch (c[1]) {
        case (char) 0xc0:
            sof0(fichier, img);
            break;
        case (char) 0xc4:
            dht(fichier, img);
            break;
        case (char) 0xd8:
        case (char) 0xd9:
            break;
        case (char) 0xda:
            sos(fichier, img);
            break;
        case (char) 0xdb:
            dqt(fichier, img);
            break;
        case (char) 0xe0:
            app0(fichier, img);
            break;
        case (char) 0xfe:
            com(fichier);
            break; 
        default:
            fprintf(stderr, "Marqueur inconnu : %x", c[1]);
            exit(EXIT_FAILURE);
    } 
}


void app0(FILE *fichier, img_t *img) {
    uint16_t length = ((uint16_t)fgetc(fichier) << 8) + fgetc(fichier);
    if (length != 16) erreur("Longueur section APP0 incorrect");
    fread(img->other->jfif, 1, 5, fichier);
    img->other->version_jfif_a = fgetc(fichier);
    img->other->version_jfif_b = fgetc(fichier);
    fseek(fichier, 7, SEEK_CUR);
}


void com(FILE *fichier) {
    uint16_t length = ((uint16_t)fgetc(fichier) << 8) + fgetc(fichier);
    fseek(fichier, length-2, SEEK_CUR);
}


void dqt(FILE *fichier, img_t *img) {
    uint16_t length = ((uint16_t)fgetc(fichier) << 8) + fgetc(fichier);
    if ((length-2) % 65 != 0) erreur("Longueur section DQT incorrect");
    for (uint8_t n=1; n<=(length-2)/65; n++) {
        uint8_t octet = fgetc(fichier);
        uint8_t precision = octet >> 4;
        if (precision != 0) erreur("Format incorrect (DQT) : la précision de la table de quantification doit être de 8 bits pour le baseline");
        uint8_t id_quant = octet & 0b1111;
        if (id_quant > 3) erreur("Format incorrect (DQT) : l'indice de la table de quantification doit être entre 0 et 3");
        if (img->qtables[id_quant] == NULL) {
            img->qtables[id_quant] = malloc(sizeof(qtable_prec_t));
            img->qtables[id_quant]->qtable = malloc(sizeof(qtable_t));
        }
        img->qtables[id_quant]->precision = precision;
        for (uint8_t i=0; i<64; i++) {
            img->qtables[id_quant]->qtable->data[i] = fgetc(fichier);
        }
    }
}


void sof0(FILE *fichier, img_t *img) {
    uint16_t length = ((uint16_t)fgetc(fichier) << 8) + fgetc(fichier);
    img->comps->precision_comp = fgetc(fichier);
    img->height = ((uint16_t)fgetc(fichier) << 8) + fgetc(fichier);
    img->width = ((uint16_t)fgetc(fichier) << 8) + fgetc(fichier);
    uint8_t nb_comp = fgetc(fichier);
    if (length != 8+3*nb_comp) erreur("Longueur section SOF0 incorrect");
    img->comps->nb = nb_comp;
    for (int i=0; i<=nb_comp-1; i++) {
        uint8_t id_comp = fgetc(fichier);
        uint8_t sampling = fgetc(fichier);
        uint8_t id_quant = fgetc(fichier);
        img->comps->comps[i] = malloc(sizeof(idcomp_t));
        img->comps->comps[i]->idc = id_comp;
        img->comps->comps[i]->hsampling = sampling >> 4;
        img->comps->comps[i]->vsampling = sampling & 0b1111;
        img->comps->comps[i]->idq = id_quant;
    }
}


void remplir_huff(huffman_tree_t *htable, uint16_t nb_symb, uint8_t lengths[nb_symb], uint8_t symb[nb_symb]) {
    list_t *list = init_list();
    couple_tree_depth_t* couple = malloc(sizeof(couple_tree_depth_t));
    couple->tree = htable;
    couple->depth = 0;
    insert_list(list, couple);
    huffman_tree_t *tree;
    uint8_t depth;
    uint8_t num = 0;
    while (num < nb_symb && !list_vide(list)) {
        couple_tree_depth_t *couple = extract_list(list);
        tree = couple->tree;
        depth = couple->depth;
        free(couple);
        if (depth == lengths[num]) {
            tree->symb = symb[num];
            num++;
        }
        else {
            tree->gauche = calloc(1,sizeof(huffman_tree_t));
            tree->droit = calloc(1,sizeof(huffman_tree_t));
            couple_tree_depth_t *couple1 = malloc(sizeof(couple_tree_depth_t));
            couple_tree_depth_t *couple2 = malloc(sizeof(couple_tree_depth_t));
            couple1->tree = tree->gauche;
            couple2->tree = tree->droit;
            couple1->depth = depth + 1;
            couple2->depth = depth + 1;
            insert_list(list, couple1);
            insert_list(list, couple2);
        }
    }
    if (list_vide(list)) erreur("Format incorrect (DHT) : code huffman incorrect");
    while (!list_vide(list)) {
        couple_tree_depth_t *couple = extract_list(list);
        free(couple);
    }
    free(list);
}


void dht(FILE *fichier, img_t *img) {
    uint64_t debut = ftell(fichier);
    uint16_t length = ((uint16_t)fgetc(fichier) << 8) + fgetc(fichier);

    while ((uint64_t) ftell(fichier) < debut+length) {
        uint8_t octet = fgetc(fichier);
        if ((octet & 0b11100000) != 0) erreur("Format incorrect (DHT) : les 3 premiers bits de la section doivent valoir 0");
	    bool is_dc = (octet & 0b00010000) == 0;
        uint8_t id_huff = octet & 0b1111;
        if (id_huff > 3) erreur("Format incorrect (DHT) : l'indice de la table de huffman doit être entre 0 et 3");
        
        uint8_t longueur_codes_brutes[16];
        fread(longueur_codes_brutes, 1, 16, fichier);
        uint16_t nb_codes = 0;
        for (uint8_t i=0; i<16; i++) {
            nb_codes += longueur_codes_brutes[i];
        }
        if (nb_codes >= 256) erreur("Format incorrect (DHT) : il y a plus de 256 symboles dans la table de Huffman");
        uint8_t longueur_codes_formatees[nb_codes];
        uint8_t i=0;
        for (uint8_t longueur=1; longueur<=16; longueur++) {
            for (uint16_t nb_longueur=0; nb_longueur<longueur_codes_brutes[longueur-1]; nb_longueur++) {
                longueur_codes_formatees[i] = longueur;
		        i++;
            }
        }
        uint8_t symb[nb_codes];
        fread(&symb, 1, nb_codes, fichier);

        huffman_tree_t **htables;
        if (is_dc) htables = img->htables->dc;
        else htables = img->htables->ac;
        if (htables[id_huff] != NULL) erreur("Format incorrect (DHT) : plusieurs table de Huffman ont le même indice");
        htables[id_huff] = calloc(1,sizeof(huffman_tree_t));

        remplir_huff(htables[id_huff], nb_codes, longueur_codes_formatees, symb);
    }
    if ((uint64_t) ftell(fichier) != debut+length) erreur("Longueur section DHT incorrect");
}


void sos(FILE *fichier, img_t *img) {
    uint16_t length = ((uint16_t)fgetc(fichier) << 8) + fgetc(fichier);
    uint8_t nb_comp = fgetc(fichier);
    if (nb_comp != img->comps->nb) erreur("Format incorrect (SOS) : le nombre de composantes est différent de celui défini dans SOF0)");
    if (length != 6+2*nb_comp) erreur("Longueur section SOS incorrect");
    for (uint8_t i=0; i<=nb_comp-1; i++) {
        uint8_t id_comp = fgetc(fichier);
        uint8_t id_huff = fgetc(fichier);
        img->comps->ordre[i] = id_comp;
        uint8_t j = 0;
        while (img->comps->comps[j]->idc != id_comp) {j++;} // attention si id_comp pas dans comps
        img->comps->comps[j]->idhdc = id_huff >> 4;
        img->comps->comps[j]->idhac = id_huff & 0b1111;
    }
    img->other->ss = fgetc(fichier);
    img->other->se = fgetc(fichier);
    uint8_t a = fgetc(fichier);
    img->other->ah = a >> 4;
    img->other->al = a & 0b1111;
    img->sosdone = true;
}
