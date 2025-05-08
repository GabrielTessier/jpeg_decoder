#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <stdarg.h>
#include <math.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/stat.h>
#include "jpeg2ppm.h"
#include "iqzz.h"
#include "idct.h"
#include "vld.h"
#include "ycc2rgb.h"
#include "entete.h"
#include "options.h"

char *execname;
int verbose;
int print_time;
char *filepath;
char *outfile;
uint64_t timer;
uint64_t abs_timer;

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

void print_v(const char* format, ...) {
  if (verbose) {
    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);
  }
}

void print_hufftable(char* acu, huffman_tree_t* tree) {
  if (verbose) {
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
}

uint64_t cast_time(struct timeval time) {
  return time.tv_sec*1000000 + time.tv_usec;
}

void start_timer() {
  if (print_time) {
    struct timeval t;
    gettimeofday(&t, NULL);
    timer = cast_time(t);
    abs_timer = timer;
  }
}

void print_timer(char* text) {
  if (print_time) {
    struct timeval t;
    gettimeofday(&t, NULL);
    uint64_t tt = cast_time(t);
    fprintf(stdout, "%s : %f s\n", text, (float) (tt-timer)/1000000);
    timer = tt;
  }
}

// Fonction principale

int main(int argc, char *argv[]) {
  // Vérification arguments
  execname = argv[0];
  set_option(argc, argv);
  if (filepath == NULL) print_help();
  if (access(filepath, R_OK)) erreur("Pas de fichier '%s'", filepath);
  if (outfile != NULL) {
    char* outfile_copy = malloc(sizeof(char)*(strlen(outfile)+1));
    strcpy(outfile_copy, outfile);
    char* folder = dirname(outfile_copy);
    struct stat sb;
    if (stat(folder, &sb) == -1) {
      print_v("création du dosier %s\n", folder);
      mkdir(folder, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    }
    print_v("outfile : %s\n", outfile);
    free(outfile_copy);
  }
    
  // Ouverture fichier
  char *fileext  = strrchr(filepath, '.') + 1; // extension du fichier
  if ((fileext == NULL) || (!strcmp(fileext, "jpeg") && !strcmp(fileext, "jpg"))) {
    erreur("Erreur : mauvaise extension de fichier.");
  }
  FILE *fichier = fopen(filepath, "r");
  if (fichier == NULL)
    erreur("Erreur : fichier introuvable.");
  // Parsing de l'en-tête
  start_timer();
  img_t *img = decode_entete(fichier);
  print_timer("Décodage entête");

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
  int nbblocX = ceil((float)img->width / 8);
  int nbbloc = ceil((float)img->height / 8) * nbblocX;
  bloctu8_t ***ycc = (bloctu8_t ***) malloc(sizeof(bloctu8_t **)*nbbloc);

  float ****stockage_coef = calc_coef();

  uint8_t off = 0;
  uint64_t debut = ftell(fichier);
  int16_t *dc_prec = (int16_t*) calloc(nbcomp, sizeof(int16_t));
  for (int i=0; i<nbbloc; i++) {
    ycc[i] = (bloctu8_t**) malloc(sizeof(bloctu8_t*)*nbcomp);
    for (int k=0; k<nbcomp; k++) {
      huffman_tree_t *hdc = NULL;
      huffman_tree_t *hac = NULL;
      qtable_prec_t *qtable = NULL;

      hdc = img->htables->dc[img->comps->comps[k]->idhdc];
      hac = img->htables->ac[img->comps->comps[k]->idhac];
      qtable = img->qtables[img->comps->comps[k]->idq];
      
      if (hdc == NULL) erreur("Pas de table de huffman pour la composante %d\n", k);
      if (qtable == NULL) erreur("Pas de table de quantification pour la composante %d\n", k);
 
      
      blocl16_t *bloc = decode_bloc_acdc(fichier, hdc, hac, dc_prec+k, &debut, &off);
      blocl16_t *bloc_iq = iquant(bloc, qtable->qtable);
      bloct16_t *bloc_zz = izz(bloc_iq);
      bloctu8_t *bloc_idct = idct(bloc_zz, stockage_coef);
      ycc[i][k] = bloc_idct;

      free(bloc);
      free(bloc_iq);
      free(bloc_zz);
    }
  }
  free(dc_prec);

  fclose(fichier);

  char *filename;
  char *fullfilename;
  if (outfile == NULL) {
    filename = filepath;
    *(strrchr(filename, '.')) = 0;
    fullfilename = (char*) malloc(sizeof(char)*(strlen(filename)+5));
    strcpy(fullfilename, filename);
    if (nbcomp == 1) strcat(fullfilename, ".pgm");
    else if (nbcomp == 3) strcat(fullfilename, ".ppm");
  } else {
    fullfilename = outfile;
  }
  if (nbcomp == 1) {
    FILE *outfile = fopen(fullfilename, "w+");
    fprintf(outfile, "P5\n");   // Magic number
    fprintf(outfile, "%d %d\n", img->width, img->height); // largeur, hateur
    fprintf(outfile, "255\n"); // nombre de valeurs d'une composante de couleur
    // Impression des pixels
    print_v("width: %d, height: %d\n", img->width, img->height);
    for (int y=0; y<img->height; y++) {
      for (int x=0; x<img->width; x++) {
        // On print le pixel de coordonnée (x,y)
        int bx = x/8;  // bx-ieme bloc horizontalement
        int by = y/8;  // by-ieme bloc verticalement
        int px = x%8;
        int py = y%8;  // le pixel est à la coordonnée (px,py) du blob (bx,by)
        fprintf(outfile, "%c", ycc[by*nbblocX + bx][0]->data[px][py]);
      }
    }
    fclose(outfile);
  } else if (nbcomp == 3) {     // YCbCr -> RGB
    FILE *outfile = fopen(fullfilename, "w+");
    fprintf(outfile, "P6\n");   // Magic number
    fprintf(outfile, "%d %d\n", img->width, img->height); // largeur, hateur
    fprintf(outfile, "255\n"); // nombre de valeurs d'une composante de couleur
    // Impression des pixels
    print_v("width: %d, height: %d\n", img->width, img->height);
    for (int y=0; y<img->height; y++) {
      for (int x=0; x<img->width; x++) {
        // On print le pixel de coordonnée (x,y)
        int bx = x/8;  // bx-ieme bloc horizontalement
        int by = y/8;  // by-ieme bloc verticalement
        int px = x%8;
        int py = y%8;  // le pixel est à la coordonnée (px,py) du blob (bx,by)
	rgb_t *rgb = ycc2rgb_pixel(ycc[by*nbblocX + bx][0]->data[px][py], ycc[by*nbblocX + bx][1]->data[px][py], ycc[by*nbblocX + bx][2]->data[px][py]);
        fprintf(outfile, "%c%c%c", rgb->r, rgb->g, rgb->b);
	free(rgb);
      }
    }
    fclose(outfile);
  }
  print_timer("Affichage pixel");
  if (print_time) {
    struct timeval t;
    gettimeofday(&t, NULL);
    fprintf(stdout, "temps total : %f s\n", (float) (cast_time(t)-abs_timer)/1000000);
  }
  return 0;
}
