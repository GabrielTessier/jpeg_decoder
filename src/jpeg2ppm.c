#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>

#include <entete.h>
#include <options.h>
#include <utils.h>
#include <timer.h>
#include <baseline.h>
#include <progressive.h>
#include <erreur.h>
#include <jpeg2ppm.h>

extern all_option_t all_option;

// Fonction pour :
// - initialiser les options
// - vérifier l'existance de l'image en entrée
// - créer le dossier contenant l'image de sortie
static erreur_t verif_option_io(int argc, char **argv);

static erreur_t verif_option_io(int argc, char **argv) {
   // On set les options
   all_option.execname = argv[0];
   erreur_t err = set_option(&all_option, argc, argv);
   if (err.code) return err;

   if (all_option.print_help) {
      print_help(&all_option);
      return (erreur_t) {.code = SUCCESS};
   }

   // Vérification qu'une image est passée en paramètre
   if (all_option.filepath == NULL) {
      print_help(&all_option);
      return (erreur_t) {.code = ERR_INVALID_FILE_PATH, .com = "Pas de fichier jpeg/jpg", .must_free = false};
   }
   if (access(all_option.filepath, R_OK)) {
      char *com = (char*) calloc(18+strlen(all_option.filepath), sizeof(char));
      sprintf(com, "Pas de fichier '%s'", all_option.filepath);
      return (erreur_t) {.code = ERR_INVALID_FILE_PATH, .com = com, .must_free = true};
   }
   // Création du dossier contenant l'image ppm en sortie
   if (all_option.outfile != NULL) {
      char *outfile_copy = malloc(sizeof(char) * (strlen(all_option.outfile) + 1));
      strcpy(outfile_copy, all_option.outfile);
      char *folder = dirname(outfile_copy);
      struct stat sb;
      if (stat(folder, &sb) == -1) {
         print_v("Création du dosier %s\n", folder);
         mkdir(folder, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
      }
      free(outfile_copy);
   }
   
   return (erreur_t) {.code = SUCCESS};
}

int main(int argc, char *argv[]) {
   // Initialisation du timer
   my_timer_t global_timer;
   init_timer(&global_timer);
   start_timer(&global_timer);
   
   erreur_t err = verif_option_io(argc, argv);
   if (err.code) {
      print_erreur(err);
      return err.code;
   }
   if (all_option.print_help) return 0;

   FILE *fichier;
   err = ouverture_fichier_in(&fichier);
   if (err.code) {
      print_erreur(err);
      return err.code;
   }

   // Parsing de l'entête
   my_timer_t timer_entete;
   init_timer(&timer_entete);
   start_timer(&timer_entete);
   // Initialisation de img
   img_t *img = init_img();
   err = decode_entete(fichier, true, img);
   if (err.code) {
      free_img(img);
      print_erreur(err);
      return err.code;
   }
   print_timer("Décodage entête", &timer_entete);

   if (img->section->num_sof != 0 && img->section->num_sof != 2) {
      char *str = (char*) malloc(sizeof(char)*21);
      sprintf(str, "sof%d non supporté", img->section->num_sof);
      erreur_t err = {.code = ERR_SOF_BAD, .com = str, .must_free = true};
      print_erreur(err);
      free_img(img);
      fclose(fichier);
      return err.code;
   }

   if (all_option.verbose) {
      char *out_file = out_file_name(img->comps->nb, 0);
      printf("Outfile : %s\n", out_file);
      free(out_file);
      printf("Taille de l'image : %d x %d\n", img->width, img->height);
      if (img->section->num_sof == 0) printf("Décodage baseline\n");
      if (img->section->num_sof == 2) printf("Décodage progressif\n");
      if (img->com->nb != 0) {
         printf("Commentaire : \n");
         for (int i=0; i<img->com->nb; i++) {
            printf("%s\n", img->com->com[i]);
         }
      }
   }

   if (img->section->num_sof == 0) err = decode_baseline_image(fichier, img);
   else if (img->section->num_sof == 2) err = decode_progressive_image(fichier, img);
   if (err.code) {
      free_img(img);
      print_erreur(err);
      return err.code;
   }

   fclose(fichier);

   // Free entête
   free_img(img);

   stop_timer(&global_timer);
   print_timer("Temps total", &global_timer);

   return 0;
}
