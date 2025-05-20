#pragma once

#include <stdbool.h>


// Gestion des erreurs

// Codes d'erreur
enum erreur_code_e {
   SUCCESS = 0,

   // erreurs vld
   ERR_0XFF00,
   ERR_AC_BAD,
   ERR_DC_BAD,
   ERR_SOF_BAD,
   ERR_HUFF_CODE_1,
   ERR_DIFF_AH_AL,

   // erreurs options
   ERR_PARAM,
   ERR_OPT,

   // erreurs baseline/progressif
   ERR_NO_HT,
   ERR_NO_QT,
   
   // erreurs jpeg2ppm
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
   
   ERR_DQT_LEN,
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
   
   ERR_SEVERAL_SOF,
   ERR_SOF_PRECISION,
   ERR_SOF_LEN,
   ERR_NO_SOF,

   ERR_SEVERAL_SOI,
   ERR_NO_SOI,
   
   ERR_SOS_COMP_ID,
   ERR_SOS_SS,
   ERR_SOS_SE,
   ERR_SOS_AH,
   ERR_SOS_AL,
   ERR_SOS_NB_COMP,
   ERR_SOS_LEN,
};
typedef enum erreur_code_e erreur_code_t;

struct erreur_s {
   erreur_code_t code; // code d'erreur
   char *com;	       // commentaire sur l'erreur à remonter
   bool must_free;     // si le commentaire a été malloc
};
typedef struct erreur_s erreur_t;

// Affiche le code et le commentaire de <err> sur la sortie d'erreur
void print_erreur(const erreur_t err);
