#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>

#include <iqzz.h>
#include <idct.h>
#include <idct_opt.h>
#include <upsampler.h>
#include <vld.h>
#include <ycc2rgb.h>
#include <entete.h>
#include <options.h>
#include <utils.h>
#include <jpeg2ppm.h>

all_option_t all_option;

// Décode un bloc
// fichier       : le fichier image
// img           : la structure contenant toutes les données de l'image (obtenue à partir de l'entête)
// stockage_coef : le tableau 8x8x8x8 des coefficients utilisés dans l'iDCT
// comp          : indice de la composante
// dc_prec       : tableau contenant les DC précédents pour chaque composante
// *off          : pointeur vers l'entier contenant l'offset de l'octet entrain d'être lu (on lit bit par bit)
// timerBloc     : tableau contenant les timers pour les différentes parties du décodage
static bloctu8_t *decode_bloc(FILE* fichier, img_t *img, float stockage_coef[8][8][8][8], int comp, int16_t *dc_prec, uint8_t *off, uint64_t timerBloc[4]) {
   // On récupère les tables de Huffman et de quantification pour la composante courante
   huffman_tree_t *hdc = NULL;
   huffman_tree_t *hac = NULL;
   qtable_prec_t *qtable = NULL;
   hdc = img->htables->dc[img->comps->comps[comp]->idhdc];
   hac = img->htables->ac[img->comps->comps[comp]->idhac];
   qtable = img->qtables[img->comps->comps[comp]->idq];

   // S'il manque une table on exit avec une erreur
   if (hdc == NULL) erreur("Pas de table de huffman pour la composante %d\n", comp);
   if (hac == NULL) erreur("Pas de table de huffman pour la composante %d\n", comp);
   if (qtable == NULL) erreur("Pas de table de quantification pour la composante %d\n", comp);

   // On décode un bloc de l'image (et on cronomètre le temps)
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
   // On fait la quantification inverse (et on chronomètre le temps)
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
   // On fait le zig-zag inverse (et on chronomètre le temps)
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
   // On fait la DCT inverse (et on chronomètre le temps)
   start_timer();
   timerBloc[2] += all_option.timer-time;
   time = all_option.timer;
   bloctu8_t *bloc_idct;
   // On choisit entre la iDCT rapide et lente en fonction des options passées en ligne de commande
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
   // On libère la mémoire des blocs créés
   free(bloc);
   free(bloc_iq);
   free(bloc_zz);
   return bloc_idct;
}

int main(int argc, char *argv[]) {
   // On set les options
   all_option.execname = argv[0];
   set_option(&all_option, argc, argv);

   // Initialisation du timer 
   init_timer();
   // Vérification qu'une image est passée en paramètre
   if (all_option.filepath == NULL) print_help(&all_option);
   if (access(all_option.filepath, R_OK)) erreur("Pas de fichier '%s'", all_option.filepath);
   // Création du dossier contenant l'image ppm en sortie
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
    
   // Ouverture du fichier avec vérification de l'extension
   char *fileext  = strrchr(all_option.filepath, '.') + 1; // extension du fichier
   if ((fileext == NULL) || !(strcmp(fileext, "jpeg")==0 || strcmp(fileext, "jpg")==0)) {
      erreur("Erreur : mauvaise extension de fichier.");
   }
   FILE *fichier = fopen(all_option.filepath, "r");
   if (fichier == NULL)
      erreur("Erreur : fichier introuvable.");
  
   // Parsing de l'entête
   start_timer();
   img_t *img = decode_entete(fichier);
   print_timer("Décodage entête");

   // N&B ou couleur
   const uint8_t nbcomp = img->comps->nb;
  
   if (all_option.verbose) {
      for (uint8_t i=0; i<nbcomp; i++) {
         print_v("Composante %d :\n", i);
         // Affichage tables de Huffman
         print_v("Huffman dc\n");
         print_hufftable(img->htables->dc[0]);
         print_v("Huffman ac\n");
         print_hufftable(img->htables->ac[0]);

         // Affichage tables de quantification
         print_v("Table de quantification : ");
         for (uint8_t i=0; i<64; i++) {
            print_v("%d, ", img->qtables[0]->qtable->data[i]);
         }
         print_v("\n");
      }
   }

   // Initialisation du tableau ycc :
   // ycc contient un tableau par composante
   // Chaque sous-tableau est de taille le nombre de bloc dans la composante (et stocke des blocs 8x8)
   bloctu8_t ***ycc = (bloctu8_t ***) malloc(sizeof(bloctu8_t **)*nbcomp);
   for (uint8_t i=0; i<nbcomp; i++) {
      uint64_t nbH = img->nbmcuH * img->comps->comps[i]->hsampling;
      uint64_t nbV = img->nbmcuV * img->comps->comps[i]->vsampling;
      ycc[i] = (bloctu8_t**) malloc(sizeof(bloctu8_t*)*nbH*nbV);
   }

   // Calcul des coefficients pour la DCT inverse (lente)
   float stockage_coef[8][8][8][8];
   if (!all_option.idct_fast) {
      start_timer();
      calc_coef(stockage_coef);
      print_timer("Calcul des coefficients de l'iDCT");
   }

   // Décodage de l'image
   start_timer();
   uint64_t timerDecodage = all_option.timer;
  
   // timerBloc : DCAC, IQ, IZZ, IDCT
   uint64_t timerBloc[4] = {0, 0, 0, 0};

   // On décode bit par bit, <off> est l'indice du bit dans l'octet en cours de lecture
   uint8_t off = 0;
  
   // Tableau contenant les dc précédant le bloc en cours de traitement (initialement 0 pour toutes les composantes)
   int16_t *dc_prec = (int16_t*) calloc(nbcomp, sizeof(int16_t));
   for (uint64_t i=0; i<img->nbMCU; i++) {
      print_v("MCU %d\n", i);
      uint64_t mcuX = i%img->nbmcuH;
      uint64_t mcuY = i/img->nbmcuH;
      for (uint8_t k=0; k<nbcomp; k++) {
         print_v("COMP %d\n", k);
         uint64_t nbH = img->nbmcuH * img->comps->comps[k]->hsampling;
         for (uint8_t by=0; by<img->comps->comps[k]->vsampling; by++) {
            for (uint8_t bx=0; bx<img->comps->comps[k]->hsampling; bx++) {
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

   // Affichage des temps d'exécution des différentes parties
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

   fclose(fichier);

   start_timer();
   // Si pas de fichier de sortie donné on le crée en remplaçant le .jpeg par .pgm ou .ppm
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
   if (nbcomp == 1) { // Noir et Blanc
      FILE *outputfile = fopen(fullfilename, "w+");
      fprintf(outputfile, "P5\n");   // Magic number
      fprintf(outputfile, "%d %d\n", img->width, img->height); // largeur, hateur
      fprintf(outputfile, "255\n"); // nombre de valeurs d'une composante de couleur
      // Impression des pixels
      print_v("width: %d, height: %d\n", img->width, img->height);
      uint64_t nb_blocH = img->nbmcuH * img->max_hsampling;
      for (uint64_t y=0; y<img->height; y++) {
         for (uint64_t x=0; x<img->width; x++) {
            // On print le pixel de coordonnée (x,y)
            uint64_t bx = x/8;  // bx-ieme bloc horizontalement
            uint64_t by = y/8;  // by-ieme bloc verticalement
            uint64_t px = x%8;
            uint64_t py = y%8;  // le pixel est à la coordonnée (px,py) du blob (bx,by)
            fprintf(outputfile, "%c", ycc[0][by*nb_blocH + bx]->data[px][py]);
         }
      }
      fclose(outputfile);
   } else if (nbcomp == 3) {     // YCbCr -> RGB
      uint8_t y_id, cb_id, cr_id;
      for (uint8_t i=0; i<nbcomp; i++) {
         if (img->comps->ordre[i] == 1) y_id = i;
         if (img->comps->ordre[i] == 2) cb_id = i;
         if (img->comps->ordre[i] == 3) cr_id = i;
      }
    
      FILE *outputfile = fopen(fullfilename, "w");
      fprintf(outputfile, "P6\n");   // Magic number
      fprintf(outputfile, "%d %d\n", img->width, img->height); // largeur, hauteur
      fprintf(outputfile, "255\n"); // nombre de valeurs d'une composante de couleur
      // Impression des pixels
      print_v("width: %d, height: %d\n", img->width, img->height);
      uint64_t nb_blocYH = img->nbmcuH * img->comps->comps[y_id]->hsampling;
      uint64_t nb_blocCbH = img->nbmcuH * img->comps->comps[cb_id]->hsampling;
      uint64_t nb_blocCrH = img->nbmcuH * img->comps->comps[cr_id]->hsampling;
      uint8_t yhf = img->max_hsampling / img->comps->comps[y_id]->hsampling;
      uint8_t yvf = img->max_vsampling / img->comps->comps[y_id]->vsampling;
      uint8_t cbhf = img->max_hsampling / img->comps->comps[cb_id]->hsampling;
      uint8_t cbvf = img->max_vsampling / img->comps->comps[cb_id]->vsampling;
      uint8_t crhf = img->max_hsampling / img->comps->comps[cr_id]->hsampling;
      uint8_t crvf = img->max_vsampling / img->comps->comps[cr_id]->vsampling;
      char *rgb = (char*) malloc(sizeof(char) * img->width * 3);
      for (uint64_t y=0; y<img->height; y++) {
         uint64_t i = 0;
         for (uint64_t x=0; x<img->width; x++) {
            // On print le pixel de coordonnée (x,y)
            uint64_t px, py;
            px = x/yhf;
            py = y/yvf;
            int8_t y_ycc = ycc[y_id][(py>>3)*nb_blocYH + (px>>3)]->data[px%8][py%8];
            px = x/cbhf;
	         py = y/cbvf;
            int8_t cb_ycc = ycc[cb_id][(py>>3)*nb_blocCbH + (px>>3)]->data[px%8][py%8];
            px = x/crhf;
	         py = y/crvf;
            int8_t cr_ycc = ycc[cr_id][(py>>3)*nb_blocCrH + (px>>3)]->data[px%8][py%8];
	         rgb_t pixel_rgb;
            ycc2rgb_pixel(y_ycc, cb_ycc, cr_ycc, &pixel_rgb);
            rgb[i*3+0] = pixel_rgb.r;
            rgb[i*3+1] = pixel_rgb.g;
            rgb[i*3+2] = pixel_rgb.b;
            i++;
         }
         fwrite(rgb, sizeof(char), img->width*3, outputfile);
      }
      free(rgb);
      fclose(outputfile);
   }
   print_timer("Affichage pixel");

   if (all_option.outfile == NULL) free(fullfilename);

   start_timer();
   // Free ycc
   for (uint8_t i=0; i<nbcomp; i++) {
      uint64_t nbH = img->nbmcuH * img->comps->comps[i]->hsampling;
      uint64_t nbV = img->nbmcuV * img->comps->comps[i]->vsampling;
      for (uint64_t j=0; j<nbH*nbV; j++) {
         free(ycc[i][j]);
      }
      free(ycc[i]);
   }
   free(ycc);

   // Free entête
   free_img(img);
   print_timer("Libération mémoire");
  
   if (all_option.print_time) {
      struct timeval t;
      gettimeofday(&t, NULL);
      fprintf(stdout, "Temps total : %f s\n", (float) (cast_time(t)-all_option.abs_timer)/1000000);
   }
   return 0;
}
