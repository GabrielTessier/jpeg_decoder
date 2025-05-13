#include <stdint.h>
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

extern all_option_t all_option;

// Décode un bloc
// fichier       : le fichier image
// img           : la structure contenant toutes les données de l'image (obtenue à partir de l'entête)
// stockage_coef : le tableau 8x8x8x8 des coefficients utilisés dans l'iDCT
// comp          : indice de la composante
// dc_prec       : tableau contenant les DC précédents pour chaque composante
// *off          : pointeur vers l'entier contenant l'offset de l'octet entrain d'être lu (on lit bit par bit)
// timerBloc     : tableau contenant les timers pour les différentes parties du décodage
static uint8_t decode_bloc(FILE* fichier, img_t *img, int comp, blocl16_t *sortie, uint8_t s_start, uint8_t s_end, int16_t *dc_prec, uint8_t *off, uint64_t timerBloc[4]) {
   // On récupère les tables de Huffman et de quantification pour la composante courante
   huffman_tree_t *hdc = NULL;
   huffman_tree_t *hac = NULL;
   qtable_prec_t *qtable = NULL;
   hdc = img->htables->dc[img->comps->comps[comp]->idhdc];
   hac = img->htables->ac[img->comps->comps[comp]->idhac];
   qtable = img->qtables[img->comps->comps[comp]->idq];

   // S'il manque une table on exit avec une erreur
   if (s_start == 0 && hdc == NULL) erreur("Pas de table de huffman DC pour la composante %d\n", comp);
   if (s_end != 0 && hac == NULL) erreur("Pas de table de huffman AC pour la composante %d\n", comp);
   if (qtable == NULL) erreur("Pas de table de quantification pour la composante %d\n", comp);

   // On décode un bloc de l'image (et on chronomètre le temps)
   uint16_t skip_bloc = decode_bloc_acdc(fichier, img->section->num_sof, hdc, hac, sortie, s_start, s_end, dc_prec+comp, off);
   if (skip_bloc != 0) skip_bloc--;
   printf("skip : %d\n", skip_bloc);
   // On fait la quantification inverse (et on chronomètre le temps)
   iquant(sortie, s_start, s_end, qtable->qtable);

   return skip_bloc;
}

static void verif_option(int argc, char **argv) {
   // On set les options
   all_option.execname = argv[0];
   set_option(&all_option, argc, argv);

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
}

static FILE *ouverture_fichier_in() {
   // Ouverture du fichier avec vérification de l'extension
   char *fileext  = strrchr(all_option.filepath, '.') + 1; // extension du fichier
   if ((fileext == NULL) || !(strcmp(fileext, "jpeg")==0 || strcmp(fileext, "jpg")==0)) {
      erreur("Erreur : mauvaise extension de fichier.");
   }
   FILE *fichier = fopen(all_option.filepath, "r");
   if (fichier == NULL) erreur("Erreur : fichier introuvable.");
   return fichier;
}

static FILE *ouverture_fichier_out(uint8_t nbcomp) {
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
   FILE *outputfile = fopen(fullfilename, "w");
   if (all_option.outfile == NULL) free(fullfilename);
   return outputfile;
}

static void print_huffman_quant_table(img_t *img) {
   if (all_option.verbose) {
      for (uint8_t i=0; i<img->comps->nb; i++) {
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
}

static void save_mcu_ligne(FILE *outputfile, img_t *img, bloctu8_t ***ycc, char *rgb, uint8_t yhf, uint8_t yvf, uint8_t y_id, uint64_t nb_blocYH, uint8_t cbhf, uint8_t cbvf, uint8_t cb_id, uint64_t nb_blocCbH, uint8_t crhf, uint8_t crvf, uint8_t cr_id, uint64_t nb_blocCrH) {
   const uint8_t nbcomp = img->comps->nb;
   for (uint64_t y=0; y<img->max_vsampling*8; y++) {
      for (uint64_t x=0; x<img->width; x++) {
	 if (nbcomp == 1) {
	    // On print le pixel de coordonnée (x,y)
	    uint64_t bx = x/8;  // bx-ieme bloc horizontalement
	    uint64_t px = x%8;
	    uint64_t py = y%8;  // le pixel est à la coordonnée (px,py) du blob (bx,by)
	    fprintf(outputfile, "%c", ycc[0][bx]->data[px][py]);
	 } else if (nbcomp == 3) {
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
	    rgb[x*3+0] = pixel_rgb.r;
	    rgb[x*3+1] = pixel_rgb.g;
	    rgb[x*3+2] = pixel_rgb.b;
	 }
      }
      if (nbcomp == 3) fwrite(rgb, sizeof(char), img->width*3, outputfile);
   }
}

int main(int argc, char *argv[]) {
   verif_option(argc, argv);
   
   // Initialisation du timer 
   init_timer();
    
   FILE *fichier = ouverture_fichier_in();
  
   // Parsing de l'entête
   start_timer();
   // Initialisation de img
   img_t *img = init_img();
   decode_entete(fichier, true, img);
   print_timer("Décodage entête");

   // N&B ou couleur
   const uint8_t nbcomp = img->comps->nb;
  
   print_huffman_quant_table(img);

   // Initialisation du tableau ycc :
   // ycc contient un tableau par composante
   // Chaque sous-tableau est de taille le nombre de bloc dans la composante (et stocke des blocs 8x8)
   blocl16_t ***sortieq = (blocl16_t***) malloc(sizeof(blocl16_t**)*nbcomp);
   for (uint8_t i=0; i<nbcomp; i++) {
      uint64_t nbH = img->nbmcuH * img->comps->comps[i]->hsampling;
      uint64_t nbV = img->nbmcuV * img->comps->comps[i]->vsampling;
      sortieq[i] = (blocl16_t**) malloc(nbH*nbV*sizeof(blocl16_t));
      for (uint64_t j=0; j<nbH*nbV; j++) {
	 sortieq[i][j] = (blocl16_t*) calloc(1, sizeof(blocl16_t));
      }
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
   uint8_t y_id, cb_id, cr_id, yhf, yvf, cbhf, cbvf, crhf, crvf;
   uint64_t nb_blocYH, nb_blocCbH, nb_blocCrH;
   char *rgb;
   if (nbcomp == 3) {
      for (uint8_t i=0; i<nbcomp; i++) {
	 if (img->comps->comps[0]->idc == img->comps->ordre[i]) y_id = i;
	 if (img->comps->comps[1]->idc == img->comps->ordre[i]) cb_id = i;
	 if (img->comps->comps[2]->idc == img->comps->ordre[i]) cr_id = i;
      }
      nb_blocYH = img->nbmcuH * img->comps->comps[y_id]->hsampling;
      nb_blocCbH = img->nbmcuH * img->comps->comps[cb_id]->hsampling;
      nb_blocCrH = img->nbmcuH * img->comps->comps[cr_id]->hsampling;
      yhf = img->max_hsampling / img->comps->comps[y_id]->hsampling;
      yvf = img->max_vsampling / img->comps->comps[y_id]->vsampling;
      cbhf = img->max_hsampling / img->comps->comps[cb_id]->hsampling;
      cbvf = img->max_vsampling / img->comps->comps[cb_id]->vsampling;
      crhf = img->max_hsampling / img->comps->comps[cr_id]->hsampling;
      crvf = img->max_vsampling / img->comps->comps[cr_id]->vsampling;
      rgb = (char*) malloc(sizeof(char) * img->width * 3);
   }

   while (!img->section->eoi_done) {
      // Tableau contenant les dc précédant le bloc en cours de traitement (initialement 0 pour toutes les composantes)
      int16_t *dc_prec = (int16_t*) calloc(nbcomp, sizeof(int16_t));
      uint8_t off = 0;
      uint16_t *skip_blocs = (uint16_t*) calloc(nbcomp, sizeof(uint16_t));
      for (uint64_t i=0; i<img->nbMCU; i++) {
	 print_v("MCU %d\n", i);
	 uint64_t mcuX = i%img->nbmcuH;
	 uint64_t mcuY = i/img->nbmcuH;
	 for (uint8_t k=0; k<nbcomp; k++) {
	    uint8_t idcomp = img->comps->ordre[k];
	    if (idcomp == 0) break;
	    uint8_t indice_comp = 0;
	    for (uint8_t c=0; c<nbcomp; c++) {
	       if (img->comps->comps[c]->idc == idcomp) {
		  indice_comp = c;
		  break;
	       }
	    }
	    printf("comp : %d\n", indice_comp);
	    print_v("COMP %d\n", indice_comp);
	    uint64_t nbH = img->nbmcuH * img->comps->comps[indice_comp]->hsampling;
	    for (uint8_t by=0; by<img->comps->comps[indice_comp]->vsampling; by++) {
	       for (uint8_t bx=0; bx<img->comps->comps[indice_comp]->hsampling; bx++) {
		  if (skip_blocs[indice_comp] == 0) {
		     print_v("BLOC %d\n", by*img->comps->comps[indice_comp]->hsampling+bx);
		     uint64_t blocX = mcuX*img->comps->comps[indice_comp]->hsampling + bx;
		     uint64_t blocY = mcuY*img->comps->comps[indice_comp]->vsampling + by;
		     uint16_t skip_bloc = decode_bloc(fichier, img, indice_comp, sortieq[indice_comp][blocY*nbH + blocX], img->other->ss, img->other->se, dc_prec, &off, timerBloc);
		     skip_blocs[indice_comp] = skip_bloc;
		  } else {
		     skip_blocs[indice_comp]--;
		  }
	       }
	    }
	 }
      }
      free(skip_blocs);
      fseek(fichier, 1, SEEK_CUR);

      FILE *outputfile = ouverture_fichier_out(nbcomp);

      // On décode bit par bit, <off> est l'indice du bit dans l'octet en cours de lecture
      if (nbcomp == 1) fprintf(outputfile, "P5\n");
      else if (nbcomp == 3) fprintf(outputfile, "P6\n");
      else erreur("Pas trois composante");
      fprintf(outputfile, "%d %d\n", img->width, img->height); // largeur, hateur
      fprintf(outputfile, "255\n");

      // Initialisation du tableau ycc :
      // ycc contient un tableau par composante
      // Chaque sous-tableau est de taille le nombre de bloc dans la composante (et stocke des blocs 8x8)
      bloctu8_t ***ycc = (bloctu8_t ***) malloc(sizeof(bloctu8_t **)*nbcomp);
      for (uint8_t i=0; i<nbcomp; i++) {
	 uint64_t nbH = img->nbmcuH * img->comps->comps[i]->hsampling;
	 uint64_t nbV = img->comps->comps[i]->vsampling;
	 ycc[i] = (bloctu8_t**) calloc(nbH*nbV, sizeof(bloctu8_t*));
      }

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
		  uint64_t blocX = mcuX*img->comps->comps[k]->hsampling + bx;
		  uint64_t blocY = mcuY*img->comps->comps[k]->vsampling + by;
		  bloct16_t *bloc_zz = izz(sortieq[k][blocY*nbH + blocX]);
		  bloctu8_t *bloc_idct;
		  if (all_option.idct_fast) bloc_idct = idct_opt(bloc_zz);
		  else bloc_idct = idct(bloc_zz, stockage_coef);
		  free(bloc_zz);
		  if (ycc[k][by*nbH + blocX] != NULL) free(ycc[k][by*nbH + blocX]);
		  ycc[k][by*nbH + blocX] = bloc_idct;
	       }
	    }
	 }
	 if (i%img->nbmcuH == img->nbmcuH-1) {  // affichage une ligne de mcu
	    save_mcu_ligne(outputfile, img, ycc, rgb, yhf, yvf, y_id, nb_blocYH, cbhf, cbvf, cb_id, nb_blocCbH, crhf, crvf, cr_id, nb_blocCrH);
	 }
      }

      fclose(outputfile);
      free(dc_prec);
      // Free ycc
      for (uint8_t i=0; i<nbcomp; i++) {
	 uint64_t nbH = img->nbmcuH * img->comps->comps[i]->hsampling;
	 uint64_t nbV = img->comps->comps[i]->vsampling;
	 for (uint64_t j=0; j<nbH*nbV; j++) {
	    free(ycc[i][j]);
	 }
	 free(ycc[i]);
      }
      free(ycc);
      
      decode_entete(fichier, false, img);
   }

   if (nbcomp == 3) free(rgb);

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
   
   // Free sortieq
   for (uint8_t i=0; i<nbcomp; i++) {
      uint64_t nbH = img->nbmcuH * img->comps->comps[i]->hsampling;
      uint64_t nbV = img->nbmcuV * img->comps->comps[i]->vsampling;
      for (uint64_t j=0; j<nbH*nbV; j++) {
	 free(sortieq[i][j]);
      }
      free(sortieq[i]);
   }
   free(sortieq);

   // Free entête
   free_img(img);
  
   if (all_option.print_time) {
      struct timeval t;
      gettimeofday(&t, NULL);
      fprintf(stdout, "Temps total : %f s\n", (float) (cast_time(t)-all_option.abs_timer)/1000000);
   }
   return 0;
}
