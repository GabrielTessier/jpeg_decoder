
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include "jpeg2ppm.h"
#include "iqzz.h"
#include "idct.h"
#include "vld.h"
#include "ycc2rgb.h"
#include "entete.h"

// TODO: changer l28, 32 les indices des tables de huffman

// Free les allocations de tableaux de blocs
void free_blocs(void **blocs, uint8_t nbblocs) {
  for (int i=0; i < nbblocs; i++)
    free(blocs[i]);
  free(blocs);
}

void print_hufftable(char* acu, huffman_tree_t* tree) {
  if (tree->droit == NULL && tree->gauche == NULL) {
    printf("path : %s symbol : %x\n", acu, tree->symb);
    return;
  }
  int i = strlen(acu);
  acu[i] = '0';
  print_hufftable(acu, tree->gauche);
  acu[i] = '1';
  print_hufftable(acu, tree->droit);
  acu[i] = 0;
}

int8_t* copy_arr_int16_to_int8(int16_t *tab, int nb) {
  int8_t* res = malloc(sizeof(int8_t)*nb);
  for (int i=0; i<nb; i++) {
    if (tab[i] < -128) res[i] = -128;
    else if (tab[i] > 127) res[i] = 127;
    else res[i] = (int8_t) tab[i];
  }
  return res;
}

// Fonction principale appelée
int main(int argc, char *argv[]) {
  // Vérification arguments
  if (argc != 2) 
    erreur("Usage : %s <FILEPATH>", argv[0]);
  // Ouverture fichier
  char *fileext  = strrchr(argv[1], '.') + 1; // extension du fichier
  if ((fileext == NULL) || (!strcmp(fileext, "jpeg") && !strcmp(fileext, "jpg"))) {
    erreur("Erreur : mauvaise extension de fichier.");
  }
  FILE *fichier = fopen(argv[1], "r");
  if (fichier == NULL)
    erreur("Erreur : fichier introuvable.");
  // Parsing de l'en-tête
  img_t *img = decode_entete(fichier);



  char* acu = (char*) calloc(20, sizeof(char));
  printf("huffman dc\n");
  print_hufftable(acu, img->htables->dc[0]->htable);
  printf("huffman ac\n");
  print_hufftable(acu, img->htables->ac[0]->htable);


  
  // N&B ou couleur
  const uint8_t nbcomp = img->comps->nb;
  
  // Parcours de toutes les composantes
  bloct_t ***ycc = (bloct_t ***) malloc(sizeof(bloct_t **)*nbcomp);
  int nbbloc;
  for (int k=0; k < nbcomp; k++) {
    // Décodage de DC
    nbbloc = 4;
    blocl_t **blocs = (blocl_t**) malloc(sizeof(blocl_t*)*nbbloc);
    uint8_t off = 0;

    int8_t *dc = (int8_t*) malloc(sizeof(int8_t)*nbbloc);
    // Décodage de AC
    //uint64_t debutAC = ftell(fichier)-1;
    uint64_t debut = ftell(fichier);
    for (int i=0; i < nbbloc; i++) {
      int16_t *vraidc = decodeDC(img->htables->dc[0]->htable, fichier, debut, &off, 1);
      dc[i] = (int8_t) vraidc[0] + ((i!=0)?dc[i-1]:0);
      printf("[DC %d] : %x\n", i, dc[i]);

      debut = ftell(fichier)-1;
      
      int16_t *vraiac = decodeAC(img->htables->ac[0]->htable, fichier, debut, &off);
      int8_t *ac = copy_arr_int16_to_int8(vraiac, 63);

      printf("[AC %d] ", i);
      for (int j=0; j<63; j++) printf("%x(%d), ", vraiac[j], vraiac[j]);
      printf("\n\n");
      
      free(vraiac);

      debut = ftell(fichier)-1;
      blocs[i] = (blocl_t*) malloc(sizeof(blocl_t));
      blocs[i]->data[0] = dc[i];
      memcpy(blocs[i]->data+1, ac, 63*sizeof(int8_t));
    }
    // Déquantification et zigzag
    bloct_t **blocs_iq = (bloct_t**) malloc(sizeof(bloct_t*)*nbbloc);
    for (int i=0; i < nbbloc; i++) {
      uint8_t idqtable = 0; // TODO: à modifier selon N&B/couleur et ycc
      blocs_iq[i] = iqzz(blocs[i], img->qtables->qtables[idqtable]->qtable);
      printf("[IQZZ %d] : ", i);
      for (int j=0; j<64; j++) printf("%x, ", blocs_iq[i]->data[j%8][j/8]);
      printf("\n\n");
    }
    //free_blocs((void **) blocs, nbbloc);
    // IDCT
    bloct_t **blocs_idct = (bloct_t **) malloc(sizeof(bloct_t*)*nbbloc);
    for (int i=0; i < nbbloc; i++)
      blocs_idct[i] = idct(blocs_iq[i]);
    //free_blocs((void **) blocs_iq, nbbloc);
    // Ajout de la composante
    ycc[k] = blocs_idct;
    //free_blocs((void **) blocs_iq, nbbloc);
  }
  char *filename = basename(argv[1]); // nom du fichier
  if (nbcomp == 1) {
    char *fullfilename = (char*) malloc(sizeof(char)*(strlen(filename)+5));
    strcpy(fullfilename, filename);
    strcat(fullfilename, ".pgm");
    FILE *outfile = fopen(fullfilename, "w+");
    fprintf(outfile, "P5\n");   // Magic number
    fprintf(outfile, "%d %d\n", img->width, img->height); // largeur, hateur
    fprintf(outfile, "255\n"); // nombre de valeurs d'une composante de couleur
    // Impression des pixels
    //for (int x=0; x < img->height; x++)    // id de ligne du bloc
    //  for (int i=0; i < 8; i++)            // id de ligne dans le bloc
    //    for (int y=0; y < img->width; y++) // id de colonne du bloc
    //      for (int j=0; j < 8; j++) {      // id de colonne dans le bloc
    //        fprintf(outfile, "%c", ycc[0][x*img->width + y]->data[i][j]);
    //      }
    printf("%d, %d\n", img->width, img->height);
    for (int y=0; y<img->height; y++) {
      for (int x=0; x<img->width; x++) {
	// On print le pixel de coordonée (x,y)
	int bx = x/8;  // bx-ieme bloc horizontalement
	int by = y/8;  // by-ieme bloc verticalement
	int px = x%8;
	int py = y%8;  // le pixel est à la coordonée (px,py) du blob (bx,by)
	fprintf(outfile, "%c", ycc[0][by*img->width/8 + bx]->data[px][py]);
      }
    }

    fclose(outfile);
  } else if (nbcomp == 3) {     // YCbCr -> RGB
    //bloc_rgb_t *out = ycc2rgb(*ycc[0], *ycc[1], *ycc[2]);
  }    
  fclose(fichier);  
  return 0;
}
