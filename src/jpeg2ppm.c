#include <stdint.h>
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

static erreur_t verif_option_io(int argc, char **argv) {
   // On set les options
   all_option.execname = argv[0];
   set_option(&all_option, argc, argv);

   // Vérification qu'une image est passée en paramètre
   if (all_option.filepath == NULL) print_help(&all_option);
   if (access(all_option.filepath, R_OK)) {
     char *com = (char*) calloc(18+strlen(all_option.filepath), sizeof(char));
     sprintf(com, "Pas de fichier '%s'", all_option.filepath);
     return (erreur_t) {.code = ERR_INVALID_FILE_PATH, .com = com};
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
      print_v("outfile : %s\n", all_option.outfile);
      free(outfile_copy);
   }
   
   return (erreur_t) {.code = SUCCESS, .com = NULL};
}

int main(int argc, char *argv[]) {
   erreur_t err = verif_option_io(argc, argv);
   if (err.code) {
     print_erreur(err);
     return err.code;
   }

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

   if (img->section->num_sof == 0) err = decode_baseline_image(fichier, img);
   else if (img->section->num_sof == 2) err = decode_progressive_image(fichier, img);
   else erreur("sof%d non supporté", img->section->num_sof);
   if (err.code) {
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
