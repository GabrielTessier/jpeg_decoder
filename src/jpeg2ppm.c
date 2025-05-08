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
#include "upsampler.h"
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

bloctu8_t *decode_bloc(FILE* fichier, img_t *img, int comp, int16_t *dc_prec, uint64_t *debut, uint8_t *off, float ****stockage_coef) {
  huffman_tree_t *hdc = NULL;
  huffman_tree_t *hac = NULL;
  qtable_prec_t *qtable = NULL;

  hdc = img->htables->dc[img->comps->comps[comp]->idhdc];
  hac = img->htables->ac[img->comps->comps[comp]->idhac];
  qtable = img->qtables[img->comps->comps[comp]->idq];
      
  if (hdc == NULL) erreur("Pas de table de huffman pour la composante %d\n", comp);
  if (qtable == NULL) erreur("Pas de table de quantification pour la composante %d\n", comp);
 
  blocl16_t *bloc = decode_bloc_acdc(fichier, hdc, hac, dc_prec+comp, debut, off);
  if (verbose) {
    print_v("[DC/AC] : ");
    for (int i=0; i<64; i++) {
      print_v("%x, ", bloc->data[i]);
    }
    print_v("\n");
  }
  blocl16_t *bloc_iq = iquant(bloc, qtable->qtable);
  if (verbose) {
    print_v("[IQ] : ");
    for (int i=0; i<64; i++) {
      print_v("%x, ", bloc_iq->data[i]);
    }
    print_v("\n");
  }
  bloct16_t *bloc_zz = izz(bloc_iq);
  if (verbose) {
    print_v("[IZZ] : ");
    for (int i=0; i<8; i++) {
      for (int j=0; j<8; j++)
	print_v("%x, ", bloc_zz->data[j][i]);
    }
    print_v("\n");
  }
  bloctu8_t *bloc_idct = idct(bloc_zz, stockage_coef);
  if (verbose) {
    print_v("[IDCT] : ");
    for (int i=0; i<8; i++) {
      for (int j=0; j<8; j++) 
	print_v("%x, ", bloc_idct->data[j][i]);
    }
    print_v("\n");
  }
  free(bloc);
  free(bloc_iq);
  free(bloc_zz);
  return bloc_idct;
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

  if (verbose) {
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
  }
  
  // N&B ou couleur
  const uint8_t nbcomp = img->comps->nb;
  
  // Parcours de toutes les composantes
  int nbBlocYH = ceil((float)img->width / 8);
  int nbBlocYV = ceil((float)img->height / 8);
  //int nbBlocY = nbBlocYH * nbBlocYV;
  //int nbBlocYParMCU = img->comps->comps[0]->hsampling * img->comps->comps[0]->vsampling;
  //int nbMCU = nbBlocY / nbBlocYParMCU;
  int nbmcuH = ceil((float)nbBlocYH / img->comps->comps[0]->hsampling);
  int nbmcuV = ceil((float)nbBlocYV / img->comps->comps[0]->vsampling);
  int reelnbBlocYH = nbmcuH * img->comps->comps[0]->hsampling;
  //int reelnbBlocYV = nbmcuV * img->comps->comps[0]->vsampling;
  int nbMCU = nbmcuH*nbmcuV;
  bloctu8_t ***ycc = (bloctu8_t ***) malloc(sizeof(bloctu8_t **)*nbcomp);
  for (int i=0; i<nbcomp; i++) {
    int nbH = nbmcuH * img->comps->comps[i]->hsampling;
    int nbV = nbmcuV * img->comps->comps[i]->vsampling;
    ycc[i] = (bloctu8_t**) malloc(sizeof(bloctu8_t*)*nbH*nbV);
  }

  float ****stockage_coef = calc_coef();

  uint8_t off = 0;
  uint64_t debut = ftell(fichier);
  int16_t *dc_prec = (int16_t*) calloc(nbcomp, sizeof(int16_t));
  for (int i=0; i<nbMCU; i++) {
    print_v("MCU %d\n", i);
    uint64_t mcuX = i%nbmcuH;
    uint64_t mcuY = i/nbmcuH;
    for (int k=0; k<nbcomp; k++) {
      print_v("COMP %d\n", k);
      uint64_t nbH = nbmcuH * img->comps->comps[k]->hsampling;
      //uint64_t nbV = nbmcuV * img->comps->comps[k]->vsampling;
      for (int by=0; by<img->comps->comps[k]->vsampling; by++) {
	for (int bx=0; bx<img->comps->comps[k]->hsampling; bx++) {
	  print_v("BLOC %d\n", by*img->comps->comps[k]->hsampling+bx);
	  bloctu8_t *bloc = decode_bloc(fichier, img, k, dc_prec, &debut, &off, stockage_coef);
	  uint64_t blocX = mcuX*img->comps->comps[k]->hsampling + bx;
	  uint64_t blocY = mcuY*img->comps->comps[k]->vsampling + by;
	  ycc[k][blocY*nbH + blocX] = bloc;
	}
      }
    }
  }
  free(dc_prec);
  for (int x=0; x < 8; x++) {
    for (int y=0; y < 8; y++) {
      for (int lambda=0; lambda < 8; lambda++) {
        free(stockage_coef[x][y][lambda]);
      }
      free(stockage_coef[x][y]);
    }
    free(stockage_coef[x]);
  }
  free(stockage_coef);

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
        fprintf(outfile, "%c", ycc[0][by*reelnbBlocYH + bx]->data[px][py]);
      }
    }
    fclose(outfile);
  } else if (nbcomp == 3) {     // YCbCr -> RGB
    // Upsampler
    bloctu8_t ***yccUP = upsampler(ycc[1], ycc[2], img);
    
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
	uint8_t y = ycc[0][by*reelnbBlocYH + bx]->data[px][py];
	uint8_t cb = yccUP[0][by*reelnbBlocYH + bx]->data[px][py];
	uint8_t cr = yccUP[1][by*reelnbBlocYH + bx]->data[px][py];
	rgb_t *rgb = ycc2rgb_pixel(y, cb, cr);
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
