#include <stdint.h>
#include <sys/stat.h>
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
#include <timer.h>
#include <baseline.h>
#include <progressive.h>
#include <jpeg2ppm.h>

extern all_option_t all_option;

static void verif_option(int argc, char **argv) {
   // On set les options
   all_option.execname = argv[0];
   set_option(&all_option, argc, argv);

   // Vérification qu'une image est passée en paramètre
   if (all_option.filepath == NULL) print_help(&all_option);
   if (access(all_option.filepath, R_OK)) erreur("Pas de fichier '%s'", all_option.filepath);
   // Création du dossier contenant l'image ppm en sortie
   if (all_option.outfile != NULL) {
      char *outfile_copy = malloc(sizeof(char) * (strlen(all_option.outfile) + 1));
      strcpy(outfile_copy, all_option.outfile);
      char *folder = dirname(outfile_copy);
      struct stat sb;
      if (stat(folder, &sb) == -1) {
         print_v("création du dosier %s\n", folder);
         mkdir(folder, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
      }
      print_v("outfile : %s\n", all_option.outfile);
      free(outfile_copy);
   }
}

static void print_huffman_quant_table(img_t *img) {
   if (all_option.verbose) {
      for (uint8_t i = 0; i < img->comps->nb; i++) {
         print_v("Composante %d :\n", i);
         // Affichage tables de Huffman
         if (img->htables->dc[i] != NULL) {
            print_v("Huffman dc\n");
            print_hufftable(img->htables->dc[i]);
         }
         if (img->htables->ac[i] != NULL) {
            print_v("Huffman ac\n");
            print_hufftable(img->htables->ac[i]);
         }

         // Affichage tables de quantification
         if (img->qtables[i] != NULL) {
            print_v("Table de quantification : ");
            for (uint8_t j = 0; j < 64; j++)
            {
               print_v("%d, ", img->qtables[i]->qtable->data[j]);
            }
            print_v("\n");
         }
      }
   }
}

int main(int argc, char *argv[]) {
   verif_option(argc, argv);

   // Initialisation du timer
   my_timer_t global_timer;
   init_timer(&global_timer);
   start_timer(&global_timer);

   FILE *fichier = ouverture_fichier_in();

   // Parsing de l'entête
   my_timer_t entete_timer;
   start_timer(&entete_timer);
   // Initialisation de img
   img_t *img = init_img();
   decode_entete(fichier, true, img);
   stop_timer(&entete_timer);
   print_timer("Décodage entête", &entete_timer);

   print_huffman_quant_table(img);

   if (img->section->num_sof == 0) decode_baseline_image(fichier, img);
   else if (img->section->num_sof == 2) decode_progressive_image(fichier, img);
   else erreur("sof%d non supporté", img->section->num_sof);

   fclose(fichier);

   // Free entête
   free_img(img);

   stop_timer(&global_timer);
   print_timer("Temps total", &global_timer);

   return 0;
}
