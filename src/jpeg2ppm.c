#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include "jpeg2ppm.h"
#include "iqzz.h"
#include "idct.h"
#include "ycc2rgb.h"

// TODO: changer l28, 32 les indices des tables de huffman

// Free les allocations de tableaux de blocs
void free_blocs(void **blocs, uint8_t nbblocs) {
  for (int i=0; i < nbblocs; i++)
    free(blocs[i]);
  free(blocs);
}

// Fonction principale appelée
int main(int argc, char *argv[]) {
  // Vérification arguments
  if (argc != 2) {
    fprintf(stderr, "Usage : ./jpeg2ppm <FILEPATH>");
    return 1;
  }
  // Ouverture fichier
  char *fileext  = strrchr(argv[1], '.') + 1; // extension du fichier
  if ((fileext == NULL) || !strcmp(fileext, "jpeg") || !strcmp(fileext, "jpg")) {
    fprintf(stderr, "Erreur : mauvaise extension de fichier.");
  }
  FILE *fichier = fopen(argv[1], "r");
  if (fichier == NULL) {
    fprintf(stderr, "Erreur : fichier introuvable.");
    return 1;
  }
  // Parsing de l'en-tête
  img_t *img = decode_entete(fichier);

  // N&B ou couleur
  const uint8_t nbcomp = img->nb;
  
  // Parcours de toutes les composantes
  bloct_t **ycc[nbcomp];
  int nbbloc;
  for (int k=0; k < nbcomp; k++) {
    // Décodage de DC
    uint64_t debutDC = 0;
    nbbloc = 1;
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
    bloct_t **blocs_idct = (bloct_t **) malloc(sizeof(bloct_t*)*nbbloc);
    for (int i=0; i < nbbloc; i++)
      blocs_idct[i] = idct(blocs_iq[i]);
    free_blocs((void **) blocs_iq, nbbloc);
    // Ajout de la composante
    ycc[k] = blocs_idct;
    free_blocs((void **) blocs_iq, nbbloc);
  }
  char *filename = basename(argv[1]); // nom du fichier
  if (nbcomp == 1) {
    FILE *outfile = fopen(strcat(filename, ".pgm"), "w+");
    fprintf(outfile, "P5\n");   // Magic number
    fprintf(outfile, "%d %d\n", img->width, img->height); // largeur, hateur
    fprintf(outfile, "255"); // nombre de valeurs d'une composante de couleur
    // Impression des pixels
    for (int x=0; x < img->height; x++)    // id de ligne du bloc
      for (int i=0; i < 8; i++)            // id de ligne dans le bloc
        for (int y=0; y < img->width; y++) // id de colonne du bloc
          for (int j=0; j < 8; j++) {      // id de colonne dans le bloc
            fprintf(outfile, "%c", ycc[0][x*img->width + y][i][j]);
          }
  } else if (nbcomp == 3) {     // YCbCr -> RGB
    bloct_t *out;
    ycc2rgb(*ycc[0], *ycc[1], *ycc[2]);
  }    
  fclose(fichier);  
  return 0;
}
