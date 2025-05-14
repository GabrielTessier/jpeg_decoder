#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include <vld.h>
#include <options.h>
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

void erreur(const char* text, ...) {
    fprintf(stderr, "ERREUR : ");
    va_list args;
    va_start(args, text);
    vfprintf(stderr, text, args);
    va_end(args);
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}

// Gestion des erreurs

enum erreur_code_e {
   // erreurs vld
   ERR_0XFF00,
   ERR_AC_BAD,
   ERR_SOF_BAD,
   ERR_HUFF_CODE_1,

   // erreurs options
   ERR_PARAM,
   ERR_OPT,

   // erreurs jpeg2ppm
   ERR_NO_HT,
   ERR_NO_QT,
   ERR_INVALID_FILE_PATH,
   ERR_INVALID_FILE_EXT,
   ERR_NB_COMP,
   ERR_SOF_NON_SUPPORTE,
   
   // erreurs entête
   ERR_APP0_LEN,
   ERR_NO_APP0,

   ERR_COMP_ID,
   ERR_COM_LEN,
   
   ERR_DHT_START_0,
   ERR_DHT_LEN,
   ERR_NO_DHT,
   
   ERR_DQT_PRECISION,
   ERR_DQT_ID,
   ERR_NO_DQT,

   ERR_EOI_BEFORE_SOS,
   ERR_NO_EOI,

   ERR_HUFF_BAD,
   ERR_HUFF_ID,
   ERR_HUFF_MORE_256,

   ERR_JFIF_VERSION,
   ERR_NO_JFIF,

   ERR_MARKER_BAD,
   ERR_MARKER_UNKNOWN,
   
   ERR_SOF_PRECISION,
   ERR_SOF_LEN,
   ERR_NO_SOF,

   ERR_SEVERAL_SOI,
   ERR_NO_SOI,
   
   ERR_SOS_ID,
   ERR_SOS_SS,
   ERR_SOS_SE,
   ERR_SOS_AH,
   ERR_SOS_AL,
   ERR_SOS_NB_COMP,
   ERR_SOS_LEN,
   
   ERR_SQT_LEN,
   ERR_SQT_PREC,
};
typedef enum erreur_code_e erreur_code_t;

struct erreur_s {
   erreur_code_t code;
   char *com; // commentaire sur l'erreur à remonter
};
typedef struct erreur_s erreur_t;

/* char *err_com[] = { */
/*    "[APP0] Phrase JFIF manquante dans APP0", */
/*    "[APP0] Version JFIF doit valoir 1.1", */
/*    "[SOFX] Précision composante doit valoir 8 (Baseline ou Progressive)", */
/*    "[DQT] Précision table de quantification doit valoir 0 (8 bits) (Baseline)" */
/* }; */
