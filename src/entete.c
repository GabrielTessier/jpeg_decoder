#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>


void marqueur(FILE* fichier) {
    char c[2];
    fread(&c, 1, 2, fichier);
    if (c[0] != 0xff) {
        fprintf(stderr, "Format incorrect de marqueur");
        exit(EXIT_FAILURE);
    }
    switch (c[1]) {
        case 0xc0:
            sof0(fichier);
            break;
        case 0xc4:
            dht(fichier);
            break;
        case 0xd8:
        case 0xd9:
            break;
        case 0xda:
            sos(fichier);
            break;
        case 0xdb:
            dqt(fichier);
            break;
        case 0xe0:
            app0(fichier);
            break;
        case 0xfe:
            com(fichier);
            break; 
        default:
            fprintf(stderr, "Marqueur inconnu : %s", c[1]);
            exit(EXIT_FAILURE);
    } 
}


void app0(FILE* fichier) {
    uint16_t length;
    fread(&length, 2, 1, fichier);
    char jfif[5];
    fread(&jfif, 1, 5, fichier);
    uint16_t c;
    fread(&c, 2, 1, fichier);
    if (strcmp(jfif,"JFIF") != 0 || c != 0x0101) {
        fprintf(stderr, "Format incorrect (APP0)");
        exit(EXIT_FAILURE);
    }
    fseek(fichier, 7, SEEK_CUR);
}


void com(FILE* fichier) {
    uint16_t length;
    fread(&length, 2, 1, fichier);
    fseek(fichier, length-2, SEEK_CUR);
}


void dqt(FILE* fichier) {
    uint16_t length;
    fread(&length, 2, 1, fichier);
}


void sof0(FILE* fichier) {
    uint16_t length;
    fread(&length, 2, 1, fichier);
    uint8_t precision = fgetc(fichier);
    if (precision != 8) {
        fprintf(stderr, "Format incorrect (SOF0) : précision doit valoir 8");
        exit(EXIT_FAILURE);
    }
    uint16_t height;
    fread(&height, 2, 1, fichier);
    uint16_t width;
    fread(&width, 2, 1, fichier);
    uint8_t nb_comp = fgetc(fichier);
    for (int i=1; i<=nb_comp; i++) {
        uint8_t id_comp = fgetc(fichier);
        uint8_t sampling = fgetc(fichier);
        uint8_t id_quant = fgetc(fichier);
    }
}


void dht(FILE* fichier) {
    uint16_t length;
    fread(&length, 2, 1, fichier);
}


void sos(FILE* fichier) {
    uint16_t length;
    fread(&length, 2, 1, fichier);
    uint8_t nb_comp = fgetc(fichier);
    for (int i=1; i<=nb_comp; i++) {
        uint8_t id_comp = fgetc(fichier);
        uint8_t id_huff = fgetc(fichier);
    }
    uint8_t ss = fgetc(fichier);
    if (ss != 0) {
        fprintf(stderr, "Format incorrect (SOS) : Ss doit valoir 0)");
        exit(EXIT_FAILURE);
    }
    uint8_t se = fgetc(fichier);
    if (se != 63) {
        fprintf(stderr, "Format incorrect (SOS) : Se doit valoir 63)");
        exit(EXIT_FAILURE);
    }
    uint8_t a = fgetc(fichier);
    if (a != 0) {
        fprintf(stderr, "Format incorrect (SOS) : Ah et Al doivent valoir 0)");
        exit(EXIT_FAILURE);
    }
}


