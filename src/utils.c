#include "erreur.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include <vld.h>
#include <options.h>
#include <ycc2rgb.h>
#include <entete.h>
#include <utils.h>

extern all_option_t all_option;

void print_v(const char* format, ...) {
  if (all_option.verbose) {
    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);
  }
}

static void print_hufftable_acu(char* acu, huffman_tree_t* tree) {
  if (tree->fils[0] == NULL && tree->fils[1] == NULL) {
    print_v("path : %s symbol : %x\n", acu, tree->symb);
    return;
  }
  int i = strlen(acu);
  acu[i] = '0';
  print_hufftable_acu(acu, tree->fils[0]);
  acu[i] = '1';
  print_hufftable_acu(acu, tree->fils[1]);
  acu[i] = 0;
}

void print_hufftable(huffman_tree_t* tree) {
  if (all_option.verbose) {
    char acu[30];
    for (int i=0; i<30; i++) acu[i] = 0;
    print_hufftable_acu(acu, tree);
  }
}

erreur_t ouverture_fichier_in(FILE **fichier) {
   // Ouverture du fichier avec vérification de l'extension
   char *fileext = strrchr(all_option.filepath, '.') + 1; // extension du fichier
   if ((fileext == NULL) || !(strcmp(fileext, "jpeg") == 0 || strcmp(fileext, "jpg") == 0)) {
      return (erreur_t) {.code = ERR_INVALID_FILE_EXT, .com = "Mauvaise extension de fichier."};
   }
   *fichier = fopen(all_option.filepath, "r");
   if (*fichier == NULL) {
      return (erreur_t) {.code = ERR_INVALID_FILE_PATH, .com = "Fichier introuvable."};
   }
   return (erreur_t) {.code = SUCCESS};
}

FILE *ouverture_fichier_out(uint8_t nbcomp, uint8_t nb) {
   // Si pas de fichier de sortie donné on le crée en remplaçant le .jpeg par .pgm ou .ppm
   char *filename;
   char *fullfilename;
   if (all_option.outfile == NULL) {
      filename = all_option.filepath;
      char *point = strrchr(filename, '.');
      char prec = *point;
      *point = 0;
      fullfilename = (char *)malloc(sizeof(char) * (strlen(filename) + 9));
      strcpy(fullfilename, filename);
      *point = prec;
      if (nb != 0) {
         char nb_str[4];
         sprintf(nb_str, "%d", nb);
         size_t s = strlen(fullfilename);
         fullfilename[s] = '-';
         fullfilename[s + 1] = 0;
         strcat(fullfilename, nb_str);
      }
      if (nbcomp == 1) strcat(fullfilename, ".pgm");
      else if (nbcomp == 3) strcat(fullfilename, ".ppm");
   } else {
      fullfilename = all_option.outfile;
   }
   FILE *outputfile = fopen(fullfilename, "w");
   if (all_option.outfile == NULL) free(fullfilename);
   return outputfile;
}
