#include <cstdint>
#include <stdint.h>
#include <stdio.h>
#include "jpeg2ppm.h"
#include "iqzz.h"

// TODO: changer l28, 32 les indices des tables de huffman

int main(int argc, char *argv[]) {
  // Vérification arguments
  if (argc != 2) {
    fprintf(stderr, "Usage: ./jpeg2ppm <FILEPATH>");
    return 1;
  }
  // Ouverture fichier
  FILE *fichier = fopen(argv[1], "r");
  if (fichier == NULL) {
    fprintf(stderr, "Erreur : fichier introuvable.");
    return 1;
  }
  // Parsing de l'en-tête
  img_t *img = decode_entete(fichier);

  // noir et blanc ou couleur
  uint8_t nbcomp = 1;
  
  // Parcours de toutes les composantes
  for (int k=0; k < nbcomp; k++) {
    // Décodage de DC
    uint64_t debutDC = 0;
    int nbmcu = 1;
    mcul_t **mcus = (mcul_t**) malloc(sizeof(mcul_t*)*nbmcu);
    int8_t *dc = decodeDC(img->htables->htables[0].htable, fichier, debutDC, nbmcu);
    // Décodage de AC
    uint64_t debutAC = ftell(fichier)+1;
    for (int i=0; i < nbmcu; i++) {
      int8_t *ac = decodeAC(img->htables->htables[1].htable, fichier, debutAC);
      debutAC = ftell(fichier)+1;
      mcus[i] = (mcul_t*) malloc(sizeof(mcul_t));
      mcus[i]->data[0] = dc[i];
      memcpy(mcus[i]->data+1, ac, 63*sizeof(int8_t));
    }
    // Déquantification et zigzag
    mcut_t **mcusdq = (mcut_t*) malloc(sizeof(mcut_t*)*nbmcu);
    for (int i=0; i < nbmcu; i++) {
      uint8_t idqtable = 1; // TODO: à modifier selon N&B/couleur et ycc
      mcusdq[i] = iqzz(mcus[i], img->qtables[idqtable]);
    }
    
  }  
  fclose(fichier);  
  return 0;
}
