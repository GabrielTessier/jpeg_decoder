#include <cstdint>
#include <stdint.h>
#include <stdio.h>
#include "jpeg2ppm.h"
#include "iqzz.h"

// TODO: changer l28, 32 les indices des tables de huffman

// Fonction pour free les allocations de tableaux de blocs
void free_blocs(void **blocs, uint8_t nbblocs) {
  for (int i=0; i < nbblocs; i++)
    free(blocs[i]);
  free(blocs);
}

// Fonction principale appelée
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
    int nbbloc = 1;
    blocl_t **blocs = (blocl_t**) malloc(sizeof(blocl_t*)*nbbloc);
    int8_t *dc = decodeDC(img->htables->htables[0].htable, fichier, debutDC, nbbloc);
    // Décodage de AC
    uint64_t debutAC = ftell(fichier)+1;
    for (int i=0; i < nbbloc; i++) {
      int8_t *ac = decodeAC(img->htables->htables[1].htable, fichier, debutAC);
      debutAC = ftell(fichier)+1;
      blocs[i] = (blocl_t*) malloc(sizeof(blocl_t));
      blocs[i]->data[0] = dc[i];
      memcpy(blocs[i]->data+1, ac, 63*sizeof(int8_t));
    }
    // Déquantification et zigzag
    bloct_t **blocs_iq = (bloct_t**) malloc(sizeof(bloct_t*)*nbbloc);
    for (int i=0; i < nbbloc; i++) {
      uint8_t idqtable = 1; // TODO: à modifier selon N&B/couleur et ycc
      blocs_iq[i] = iqzz(blocs[i], img->qtables[idqtable]);
    }
    free_blocs((void **) blocs, nbbloc);
    // IDCT
    bloct_t **blocs_idct = (bloct_t **) malloc(sizeof(bloc_t*)*nbbloc);
    for (int i=0; i < nbbloc; i++) {
      
 
    
    
  }  
  fclose(fichier);  
  return 0;
}
