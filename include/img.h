#pragma once

#include <stdint.h>
#include <stdbool.h>


// Structure contenant une table de quantification dans un tableau 1x64 
struct qtable_s {
   uint16_t data[64];
};
typedef struct qtable_s qtable_t;

// Structure contenant une table de quantification et sa précision
struct qtable_prec_s {
   uint8_t precision;  // Doit valoir 0 (correspond à 8 bits) pour le mode baseline
   qtable_t *qtable;   // Table de quantification
};
typedef struct qtable_prec_s qtable_prec_t;

// Structure représentant un arbre de Huffman
struct huffman_tree_s {
   // fils[0] représente le fils gauche et fils[1] le fils droit
   struct huffman_tree_s *fils[2];
   uint8_t symb;
};
typedef struct huffman_tree_s huffman_tree_t;

// Structure contenant les tables de Huffman de type DC et AC
struct htables_s {
   huffman_tree_t *dc[4];    // 4 tables maximum pour le type DC
   huffman_tree_t *ac[4];    // 4 tables maximum pour le type AC
};
typedef struct htables_s htables_t;

// Structure contenant les informations d'une seule composante de l'image
struct idcomp_s {
   uint8_t idc;        // Indice de la composante
   uint8_t hsampling;  // Facteur d'échantillonnage horizontal
   uint8_t vsampling;  // Facteur d'échantillonnage vertical
   uint8_t idhdc;      // Indice de la table de Huffman pour les coefficients DC
   uint8_t idhac;      // Indice de la table de Huffman pour les coefficients AC
   uint8_t idq;        // Indice de la table de quantification
};
typedef struct idcomp_s idcomp_t;

// Structure contenant les informations sur les composantes de l'image
struct comps_s {
   uint8_t nb;             // Nombre de composantes
   uint8_t ordre[3];       // Ordre d'apparition des composantes en fonction de leur indice (au maximum 3 composantes)
   uint8_t precision_comp; // Précision des composantes (8 bits en baseline)
   idcomp_t *comps[3];     // Informations sur chaque composante (au maximum 3 composantes)
};
typedef struct comps_s comps_t;

// Structure indiquant l'avancement du traitement de l'entête
struct section_done_s {
   bool app0_done;  // Indique si la section APP0 a été traitée
   bool sof_done;   // Indique si la section SOF a été traitée
   uint8_t num_sof; // Indique quelle section SOF a été traitée (0 pour Baseline et 2 pour Progressif)
   bool dqt_done;   // Indique si une section DQT a été traitée
   bool dht_done;   // Indique si une section DHT a été traitée
   bool sos_done;   // Indique si une section SOS a été traitée
   bool eoi_done;   // Indique si la section EOI a été traitée
};
typedef struct section_done_s section_done_t;

// Structure contenant des informations complémentaires à vérifier pour le mode baseline
struct other_s {
   char jfif[5];           // Doit valoir "JFIF\0"
   uint8_t version_jfif_x; // Doit valoir 1
   uint8_t version_jfif_y; // Doit valoir 1
   uint8_t ss;             // Doit valoir entre 0 et 63
   uint8_t se;             // Doit valoir entre ss et 63
   uint8_t ah;             // Doit valoir 0 (ah != 0 non traité)
   uint8_t al;             // Doit valoir 0 (al != 0 non traité)
};
typedef struct other_s other_t;

// Structure contenant les commentaires de l'image
struct com_s {
   int nb;     // Nombre de commentaires
   char **com; // Tableau contenant les commentaires
};
typedef struct com_s com_t;


// Structure contennant les informations de l'entête de l'image
struct img_s {
   uint16_t height;            // Hauteur
   uint16_t width;             // Largeur
   qtable_prec_t *qtables[4];  // Tables de quantification
   htables_t *htables;         // Tables de Huffman
   comps_t *comps;             // Composantes de l'image
   section_done_t *section;    // Avancement des sections 
   other_t *other;             // Autres informations à vérifier
   com_t *com;                 // Commentaires de l'image
   uint8_t max_hsampling;      // Sampling horizontale maximal
   uint8_t max_vsampling;      // Sampling verticale maximal
   uint64_t nbmcuH;            // Nombre de mcu horizontalement
   uint64_t nbmcuV;            // Nombre de mcu verticalement
   uint64_t nbMCU;             // Nombre total de mcu
};
typedef struct img_s img_t;

// Libère une table de Huffman
void free_huffman_tree(huffman_tree_t *tree);

// Libère la structure img
void free_img(img_t *img);

// Initialise une structure img
img_t* init_img();
