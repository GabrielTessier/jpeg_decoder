#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>
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

int8_t* copy_arr_int16_to_int8(int16_t *tab, int nb) {
  int8_t* res = malloc(sizeof(int8_t)*nb);
  for (int i=0; i<nb; i++) {
    if (tab[i] < -128) res[i] = -128;
    else if (tab[i] > 127) res[i] = 127;
    else res[i] = (int8_t) tab[i];
  }
  return res;
}

// options d'exécution (-v)
static int verbose = 0;
static int print_time = 0;
static time_t timer;
static time_t abs_timer;

// Retourne la liste des options dans argv
char *get_options(const int argc, const char **argv, char **parameters) {
  char *options = (char*) malloc(sizeof(char)*80);
  int idopt = 0; // position dans options
  int idpar = 0; // position dans parameters
  for (int i=0; i < argc; i++) {
    if (*(argv[i]) == '-')
      while (*(++argv[i]))
        options[idopt++] = *(argv[i]);
    else
      parameters[idpar] = (char *) argv[i];
    }
  options[idopt] = 0;
  return options;
}

void print_v(const char* format, ...) {
  if (verbose) {
    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);
  }
}

void print_hufftable(char* acu, huffman_tree_t* tree) {
  if (tree->droit == NULL && tree->gauche == NULL) {
    print_v("path : %s symbol : %x\n", acu, tree->symb);
    return;
  }
  int i = strlen(acu);
  acu[i] = '0';
  print_hufftable(acu, tree->gauche);
  acu[i] = '1';
  print_hufftable(acu, tree->droit);
  acu[i] = 0;
}


void start_timer() {
  if (print_time) {
    timer = time(NULL);
    abs_timer = timer;
  }
}

void print_timer() {
  if (print_time) {
    fprintf(stdout, "%ld\n", time(NULL) - timer);
    timer = time(NULL);
  }
}

blocl16_t **decode_acdc(int nbbloc, FILE *fichier, img_t *img) {
  blocl16_t **blocs = (blocl16_t**) malloc(sizeof(blocl16_t*)*nbbloc);
  uint8_t off = 0; // offset pour le décodage de AC, DC
  int16_t dc=0, dc_prec;
  uint64_t debut = ftell(fichier);
  for (int i=0; i < nbbloc; i++) {
    int16_t *sousdc = decodeDC(img->htables->dc[0], fichier, debut, &off, 1);
    dc_prec = dc;
    dc = sousdc[0] + ((i!=0)?dc_prec:0); // calcul du coef à partir de la différence
    // Affichage de DC
    print_v("[DC %d] : %x\n", i, dc);

    debut = ftell(fichier)-1; // début de AC
    int16_t *ac = decodeAC(img->htables->ac[0], fichier, debut, &off);
    // Affichage de AC
    print_v("[AC %d] ", i);
    for (int j=0; j<63; j++) print_v("%x(%d), ", ac[j], ac[j]);
    print_v("\n\n");

    debut = ftell(fichier)-1;
    // écriture des DC, AC
    blocs[i] = (blocl16_t*) malloc(sizeof(blocl16_t));
    blocs[i]->data[0] = dc;
    memcpy(blocs[i]->data+1, ac, 63*sizeof(int16_t));
  }
  return blocs;
}

/* blocl16_t **decode_acdc(int nbbloc, FILE *fichier, img_t *img) { */
/*   blocl16_t **blocs = (blocl16_t**) malloc(sizeof(blocl16_t*)*nbbloc); */
/*   uint8_t off = 0; // offset pour le décodage de AC, DC */
/*   int16_t *dc = (int16_t*) malloc(sizeof(int16_t)*nbbloc); */
/*   // Décodage de AC et DC */
/*   uint64_t debut = ftell(fichier); */
/*   for (int i=0; i < nbbloc; i++) { */
/*     int16_t *sousdc = decodeDC(img->htables->dc[0]->htable, fichier, debut, &off, 1); */
/*     dc[i] = sousdc[0] + ((i!=0)?dc[i-1]:0); // calcul du coef à partir de la différence */
/*     printf("[DC %d] : %x\n", i, dc[i]); */

/*     debut = ftell(fichier)-1; // début de AC */
      
/*     int16_t *ac = decodeAC(img->htables->ac[0]->htable, fichier, debut, &off); */

/*     printf("[AC %d] ", i); */
/*     for (int j=0; j<63; j++) printf("%x(%d), ", ac[j], ac[j]); */
/*     printf("\n\n"); */

/*     debut = ftell(fichier)-1; */
/*     // écriture des DC, AC */
/*     blocs[i] = (blocl16_t*) malloc(sizeof(blocl16_t)); */
/*     blocs[i]->data[0] = dc[i]; */
/*     memcpy(blocs[i]->data+1, ac, 63*sizeof(int16_t)); */
/*   } */
/*   return blocs; */
/* } */


// Fonction principale

int main(int argc, char *argv[]) {
  // Vérification arguments
  if (argc < 2) 
    erreur("Usage : %s <filepath>\nOptions :\n\t-v : verbose\n\t-t : print timers", argv[0]);
  char *filepath = argv[1];
  char **parameters = (char**) malloc(sizeof(char*)*5);
  char *options = get_options(argc, (const char**)argv, parameters);

  for (size_t i=0; i < strlen(options); i++) {
    switch (options[i]) {
    case 'v':
      verbose = 1;
      break;
    case 't':
      print_time = 1;
      break;
    default:
      fprintf(stderr, "Erreur : option '%c' inconnue", options[i]);
      erreur("Usage : %s <filepath>\nOptions :\n\t-v : verbose\n\t-t : print timers", argv[0]);
      break;
    }
  }
    
  // Ouverture fichier
  char *fileext  = strrchr(filepath, '.') + 1; // extension du fichier
  if ((fileext == NULL) || (!strcmp(fileext, "jpeg") && !strcmp(fileext, "jpg"))) {
    erreur("Erreur : mauvaise extension de fichier.");
  }
  FILE *fichier = fopen(argv[1], "r");
  if (fichier == NULL)
    erreur("Erreur : fichier introuvable.");
  // Parsing de l'en-tête
  start_timer();
  img_t *img = decode_entete(fichier);
  print_timer();

  // Affichage tables de Huffman
  char* acu = (char*) calloc(20, sizeof(char));
  print_v("huffman dc\n");
  print_hufftable(acu, img->htables->dc[0]);
  print_v("huffman ac\n");
  print_hufftable(acu, img->htables->ac[0]);
  free(acu);

  // Affichage tables de quantification
  print_v("qtable : ");
  for (int i=0; i<64; i++) {
    print_v("%d, ", img->qtables[0]->qtable->data[i]);
  }
  print_v("\n");
  
  // N&B ou couleur
  const uint8_t nbcomp = img->comps->nb;
  
  // Parcours de toutes les composantes
  bloctu8_t ***ycc = (bloctu8_t ***) malloc(sizeof(bloctu8_t **)*nbcomp);
  int nbblocX = ceil((float)img->width / 8);
  int nbbloc = ceil((float)img->height / 8) * nbblocX;
  
  for (int k=0; k < nbcomp; k++) {
    blocl16_t **blocs = decode_acdc(nbbloc, fichier, img);
    // Déquantification
    blocl16_t **blocs_iq = (blocl16_t**) malloc(sizeof(blocl16_t*)*nbbloc);
    for (int i=0; i < nbbloc; i++) {
      uint8_t idqtable = 0; // TODO: à modifier selon N&B/couleur et ycc
      blocs_iq[i] = iquant(blocs[i], img->qtables[idqtable]->qtable);
      print_v("[IQUANT %d] : ", i);
      for (int j=0; j<64; j++) printf("%x, ", blocs_iq[i]->data[j]);
      print_v("\n\n");
    }
    // Zigzag
    bloct16_t **blocs_zz = (bloct16_t**) malloc(sizeof(bloct16_t*)*nbbloc);
    for (int i=0; i < nbbloc; i++) {
      blocs_zz[i] = izz(blocs_iq[i]);
      print_v("[IZZ %d] : ", i);
      for (int j=0; j<64; j++) printf("%x, ", blocs_zz[i]->data[j%8][j/8]);
      print_v("\n\n");
    }
    //free_blocs((void **) blocs, nbbloc);
    // IDCT
    bloctu8_t **blocs_idct = (bloctu8_t **) malloc(sizeof(bloctu8_t*)*nbbloc);
    for (int i=0; i < nbbloc; i++)
      blocs_idct[i] = idct(blocs_zz[i]);
    //free_blocs((void **) blocs_iq, nbbloc);
    // Ajout de la composante
    ycc[k] = blocs_idct;
    //free_blocs((void **) blocs_iq, nbbloc);
  }
  
  char *filename = argv[1]; // nom du fichier
  *(strrchr(filename, '.')) = 0;
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
    print_v("width: %d, height: %d\n", img->width, img->height);
    for (int y=0; y<img->height; y++) {
      for (int x=0; x<img->width; x++) {
        // On print le pixel de coordonnée (x,y)
        int bx = x/8;  // bx-ieme bloc horizontalement
        int by = y/8;  // by-ieme bloc verticalement
        int px = x%8;
        int py = y%8;  // le pixel est à la coordonnée (px,py) du blob (bx,by)
        fprintf(outfile, "%c", ycc[0][by*nbblocX + bx]->data[px][py]);
      }
    }
    fclose(outfile);
  } else if (nbcomp == 3) {     // YCbCr -> RGB
    //bloc_rgb_t *out = ycc2rgb(*ycc[0], *ycc[1], *ycc[2]);
  }
  if (print_time) {
    fprintf(stdout, "temps total : %ld\n", time(NULL) - abs_timer);
  }
  fclose(fichier);  
  return 0;
}
