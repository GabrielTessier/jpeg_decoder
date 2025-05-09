#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <math.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/stat.h>
#include "jpeg2ppm.h"
#include "iqzz.h"
#include "idct.h"
#include "idct_opt.h"
#include "upsampler.h"
#include "vld.h"
#include "ycc2rgb.h"
#include "entete.h"
#include "options.h"
#include "utils.h"

all_option_t all_option;

bloctu8_t *decode_bloc(FILE* fichier, img_t *img, float ****stockage_coef, int comp, int16_t *dc_prec, uint8_t *off, uint64_t *timerBloc) {
  huffman_tree_t *hdc = NULL;
  huffman_tree_t *hac = NULL;
  qtable_prec_t *qtable = NULL;

  hdc = img->htables->dc[img->comps->comps[comp]->idhdc];
  hac = img->htables->ac[img->comps->comps[comp]->idhac];
  qtable = img->qtables[img->comps->comps[comp]->idq];
      
  if (hdc == NULL) erreur("Pas de table de huffman pour la composante %d\n", comp);
  if (qtable == NULL) erreur("Pas de table de quantification pour la composante %d\n", comp);

  start_timer();
  uint64_t time = all_option.timer;
  blocl16_t *bloc = decode_bloc_acdc(fichier, hdc, hac, dc_prec+comp, off);
  if (all_option.verbose) {
    print_v("[DC/AC] : ");
    for (int i=0; i<64; i++) {
      print_v("%x, ", bloc->data[i]);
    }
    print_v("\n");
  }
  start_timer();
  timerBloc[0] += all_option.timer-time;
  time = all_option.timer;
  blocl16_t *bloc_iq = iquant(bloc, qtable->qtable);
  if (all_option.verbose) {
    print_v("[IQ] : ");
    for (int i=0; i<64; i++) {
      print_v("%x, ", bloc_iq->data[i]);
    }
    print_v("\n");
  }
  start_timer();
  timerBloc[1] += all_option.timer-time;
  time = all_option.timer;
  bloct16_t *bloc_zz = izz(bloc_iq);
  if (all_option.verbose) {
    print_v("[IZZ] : ");
    for (int i=0; i<8; i++) {
      for (int j=0; j<8; j++)
	print_v("%x, ", bloc_zz->data[j][i]);
    }
    print_v("\n");
  }
  start_timer();
  timerBloc[2] += all_option.timer-time;
  time = all_option.timer;
  bloctu8_t *bloc_idct;
  if (all_option.idct_fast) bloc_idct = idct_opt(bloc_zz);
  else bloc_idct = idct(bloc_zz, stockage_coef);
  if (all_option.verbose) {
    print_v("[IDCT] : ");
    for (int i=0; i<8; i++) {
      for (int j=0; j<8; j++) 
	print_v("%x, ", bloc_idct->data[j][i]);
    }
    print_v("\n");
  }
  start_timer();
  timerBloc[3] += all_option.timer-time;
  time = all_option.timer;
  free(bloc);
  free(bloc_iq);
  free(bloc_zz);
  return bloc_idct;
}

// Fonction principale

int main(int argc, char *argv[]) {
  // Vérification arguments
  all_option.execname = argv[0];
  set_option(&all_option, argc, argv);

  init_timer();
  if (all_option.filepath == NULL) print_help(&all_option);
  if (access(all_option.filepath, R_OK)) erreur("Pas de fichier '%s'", all_option.filepath);
  if (all_option.outfile != NULL) {
    char* outfile_copy = malloc(sizeof(char)*(strlen(all_option.outfile)+1));
    strcpy(outfile_copy, all_option.outfile);
    char* folder = dirname(outfile_copy);
    struct stat sb;
    if (stat(folder, &sb) == -1) {
      print_v("création du dosier %s\n", folder);
      mkdir(folder, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    }
    print_v("outfile : %s\n", all_option.outfile);
    free(outfile_copy);
  }
    
  // Ouverture fichier
  char *fileext  = strrchr(all_option.filepath, '.') + 1; // extension du fichier
  if ((fileext == NULL) || (!strcmp(fileext, "jpeg") && !strcmp(fileext, "jpg"))) {
    erreur("Erreur : mauvaise extension de fichier.");
  }
  FILE *fichier = fopen(all_option.filepath, "r");
  if (fichier == NULL)
    erreur("Erreur : fichier introuvable.");
  // Parsing de l'en-tête
  start_timer();
  img_t *img = decode_entete(fichier);
  print_timer("Décodage entête");

  if (all_option.verbose) {
    // Affichage tables de Huffman
    char* acu = (char*) calloc(20, sizeof(char));
    print_v("huffman dc\n");
    print_hufftable(img->htables->dc[0]);
    print_v("huffman ac\n");
    print_hufftable(img->htables->ac[0]);
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
  int reelnbBlocYV = nbmcuV * img->comps->comps[0]->vsampling;
  int nbMCU = nbmcuH*nbmcuV;
  bloctu8_t ***ycc = (bloctu8_t ***) malloc(sizeof(bloctu8_t **)*nbcomp);
  for (int i=0; i<nbcomp; i++) {
    int nbH = nbmcuH * img->comps->comps[i]->hsampling;
    int nbV = nbmcuV * img->comps->comps[i]->vsampling;
    ycc[i] = (bloctu8_t**) malloc(sizeof(bloctu8_t*)*nbH*nbV);
  }

  float ****stockage_coef = NULL;
  if (!all_option.idct_fast) {
    start_timer();
    stockage_coef = calc_coef();
    print_timer("Calcul des coefficients de l'iDCT");
  }

  start_timer();
  uint64_t timerDecodage = all_option.timer;
  // DCAC, IQ, IZZ, IDCT
  uint64_t timerBloc[4] = {0, 0, 0, 0};
  uint8_t off = 0;
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
	  bloctu8_t *bloc = decode_bloc(fichier, img, stockage_coef, k, dc_prec, &off, timerBloc);
	  uint64_t blocX = mcuX*img->comps->comps[k]->hsampling + bx;
	  uint64_t blocY = mcuY*img->comps->comps[k]->vsampling + by;
	  ycc[k][blocY*nbH + blocX] = bloc;
	}
      }
    }
  }
  free(dc_prec);
  start_timer();
  all_option.timer -= timerBloc[0];
  print_timer("Décodage DC/AC");
  start_timer();
  all_option.timer -= timerBloc[1];
  print_timer("Décodage IQ");
  start_timer();
  all_option.timer -= timerBloc[2];
  print_timer("Décodage IZZ");
  start_timer();
  all_option.timer -= timerBloc[3];
  print_timer("Décodage IDCT");
  all_option.timer = timerDecodage;
  print_timer("Décodage complet de l'image");

  if (!all_option.idct_fast) {
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
  }

  fclose(fichier);

  start_timer();
  char *filename;
  char *fullfilename;
  if (all_option.outfile == NULL) {
    filename = all_option.filepath;
    *(strrchr(filename, '.')) = 0;
    fullfilename = (char*) malloc(sizeof(char)*(strlen(filename)+5));
    strcpy(fullfilename, filename);
    if (nbcomp == 1) strcat(fullfilename, ".pgm");
    else if (nbcomp == 3) strcat(fullfilename, ".ppm");
  } else {
    fullfilename = all_option.outfile;
  }
  if (nbcomp == 1) {
    FILE *outputfile = fopen(fullfilename, "w+");
    fprintf(outputfile, "P5\n");   // Magic number
    fprintf(outputfile, "%d %d\n", img->width, img->height); // largeur, hateur
    fprintf(outputfile, "255\n"); // nombre de valeurs d'une composante de couleur
    // Impression des pixels
    print_v("width: %d, height: %d\n", img->width, img->height);
    for (int y=0; y<img->height; y++) {
      for (int x=0; x<img->width; x++) {
        // On print le pixel de coordonnée (x,y)
        int bx = x/8;  // bx-ieme bloc horizontalement
        int by = y/8;  // by-ieme bloc verticalement
        int px = x%8;
        int py = y%8;  // le pixel est à la coordonnée (px,py) du blob (bx,by)
        fprintf(outputfile, "%c", ycc[0][by*reelnbBlocYH + bx]->data[px][py]);
      }
    }
    fclose(outputfile);
  } else if (nbcomp == 3) {     // YCbCr -> RGB
    // Upsampler
    start_timer();
    bloctu8_t ***yccUP = upsampler(ycc[1], ycc[2], img);
    print_timer("Up sampler");
    
    FILE *outputfile = fopen(fullfilename, "w+");
    fprintf(outputfile, "P6\n");   // Magic number
    fprintf(outputfile, "%d %d\n", img->width, img->height); // largeur, hateur
    fprintf(outputfile, "255\n"); // nombre de valeurs d'une composante de couleur
    // Impression des pixels
    print_v("width: %d, height: %d\n", img->width, img->height);
    for (int y=0; y<img->height; y++) {
      char *rgb = (char*) malloc(sizeof(char) * img->width * 3);
      uint64_t i = 0;
      for (int x=0; x<img->width; x++) {
        // On print le pixel de coordonnée (x,y)
        int bx = x/8;  // bx-ieme bloc horizontalement
        int by = y/8;  // by-ieme bloc verticalement
        int px = x%8;
        int py = y%8;  // le pixel est à la coordonnée (px,py) du blob (bx,by)
	uint8_t y = ycc[0][by*reelnbBlocYH + bx]->data[px][py];
	uint8_t cb = yccUP[0][by*reelnbBlocYH + bx]->data[px][py];
	uint8_t cr = yccUP[1][by*reelnbBlocYH + bx]->data[px][py];
	rgb_t *pixel_rgb = ycc2rgb_pixel(y, cb, cr);
        //fprintf(outfile, "%c%c%c", rgb->r, rgb->g, rgb->b);
	rgb[i*3+0] = pixel_rgb->r;
	rgb[i*3+1] = pixel_rgb->g;
	rgb[i*3+2] = pixel_rgb->b;
	free(pixel_rgb);
	i++;
      }
      fwrite(rgb, sizeof(char), img->width*3, outputfile);
      free(rgb);
    }
    
    // Free yccUP
    
    for (int i=0; i<2; i++) {
      for (int j=0; j<reelnbBlocYH*reelnbBlocYV; j++) {
	free(yccUP[i][j]);
      }
      free(yccUP[i]);
    }
    free(yccUP);
    
    fclose(outputfile);
  }
  print_timer("Affichage pixel");

  if (all_option.outfile == NULL) free(fullfilename);

  start_timer();
  // Free ycc
  for (int i=0; i<nbcomp; i++) {
    int nbH = nbmcuH * img->comps->comps[i]->hsampling;
    int nbV = nbmcuV * img->comps->comps[i]->vsampling;
    for (int j=0; j<nbH*nbV; j++) {
      free(ycc[i][j]);
    }
    free(ycc[i]);
  }
  free(ycc);

  // Free entete
  free_img(img);
  print_timer("Libération mémoire");
  
  if (all_option.print_time) {
    struct timeval t;
    gettimeofday(&t, NULL);
    fprintf(stdout, "temps total : %f s\n", (float) (cast_time(t)-all_option.abs_timer)/1000000);
  }
  return 0;
}
