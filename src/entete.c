#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#include <entete.h>
#include <file.h>
#include <vld.h>
#include <utils.h>


// Structure liant un noeud d'un arbre de Huffman avec sa profondeur dans l'arbre
struct couple_tree_depth_s {
   huffman_tree_t *tree;
   uint8_t depth;
};
typedef struct couple_tree_depth_s couple_tree_depth_t;


// Libère les tables de quantification
static void free_qtables(qtable_prec_t **qtables);

// Libère un noeud d'un arbre représentant une table de Huffman
static void free_huffman_tree(huffman_tree_t *tree);

// Libère les tables de Huffman
static void free_htables(htables_t *htables);

// Libère les informations sur les composantes
static void free_comps(comps_t *comps);

// Libère les informations sur l'avancement du traitement de l'entête
static void free_section_done(section_done_t *section);

// Libère les informations complémentaires
static void free_other(other_t *other);

// Indique si on a atteint la fin du fichier
static bool fichier_fini(FILE *fichier);

// Vérifie si les informations de l'entête dans la section APP0 sont conformes
static void verif_entete_app0(img_t *img);

// Vérifie si les informations de l'entête sont conformes au mode baseline
static void verif_entete_baseline(img_t *img);

// Vérifie si les informations de l'entête sont conformes au mode progressif
static void verif_entete_progressif(img_t *img);

// Calcul d'informations complémentaires sur l'image (nombre de mcu et sampling maximal)
static void calcul_image_information(img_t *img);

// Vérifie la section SOI est présente
static void soi(FILE *fichier);

// Appelle le décodage de la bonne section en fonction du marqueur 
static void marqueur(FILE *fichier, img_t *img);

// Décode la section APP0
static void app0(FILE *fichier, img_t *img);

// Décode la section COM
static void com(FILE *fichier);

// Décode la section SOF
static void sof(FILE *fichier, img_t *img);

// Décode la section DQT
static void dqt(FILE *fichier, img_t *img);

// Décode la section DHT
static void dht(FILE *fichier, img_t *img);

// Décode la section SOS
static void sos(FILE *fichier, img_t *img);


static void free_qtables(qtable_prec_t **qtables) {
   for (int i=0; i<4; i++) {
      if (qtables[i] != NULL) {
         free(qtables[i]->qtable);
         free(qtables[i]);
      }
   }
}

static void free_huffman_tree(huffman_tree_t *tree) {
   if (tree == NULL) return;
   free_huffman_tree(tree->fils[0]);
   free_huffman_tree(tree->fils[1]);
   free(tree);
}

static void free_htables(htables_t *htables) {
   for (int i=0; i<4; i++) {
      if (htables->ac[i] != NULL) free_huffman_tree(htables->ac[i]);
      if (htables->dc[i] != NULL) free_huffman_tree(htables->dc[i]);
   }
   free(htables);
}

static void free_comps(comps_t *comps) {
   for (int i=0; i<3; i++) {
      if (comps->comps[i] != NULL) free(comps->comps[i]);
   }
   free(comps);
}

static void free_section_done(section_done_t *section) {
   free(section);
}

static void free_other(other_t *other) {
   free(other);
}

void free_img(img_t *img) {
   free_qtables(img->qtables);
   free_htables(img->htables);
   free_comps(img->comps);
   free_section_done(img->section);
   free_other(img->other);
   free(img);
}

static bool fichier_fini(FILE *fichier) {
   char c1 = fgetc(fichier);
   char c2 = fgetc(fichier);
   fseek(fichier, -2, SEEK_CUR);
   if (c1 == EOF && c2 == EOF) return true;
   return false;
}

static void verif_entete_app0(img_t *img) {
   // Section APP0
   if (strcmp(img->other->jfif,"JFIF") != 0) erreur("[APP0] Phrase JFIF manquante dans APP0");
   if (img->other->version_jfif_x != 1) erreur("[APP0] Version JFIF X doit valoir 1");
   if (img->other->version_jfif_y != 1) erreur("[APP0] Version JFIF Y doit valoir 1");
}


static void verif_entete_baseline(img_t *img) {
   // Section SOF0
   if (img->comps->precision_comp != 8) erreur("[SOF0] Précision composante doit valoir 8 (Baseline)");   
   
   // Section DQT
   for (int i=0; i<4; i++) {
      if (img->qtables[i] != NULL) {
         if (img->qtables[i]->precision != 0) erreur("[DQT] Précision table de quantification doit valoir 0 (8 bits) (Baseline)");
      }
   }

   // Section DHT
   // En baseline, les indices des tables de Huffman doivent être 0 ou 1
   if (img->htables->dc[2] != NULL) erreur("[DHT] Indice table de Huffman DC doit valoir 0 ou 1");
   if (img->htables->dc[3] != NULL) erreur("[DHT] Indice table de Huffman DC doit valoir 0 ou 1");
   if (img->htables->ac[2] != NULL) erreur("[DHT] Indice table de Huffman AC doit valoir 0 ou 1");
   if (img->htables->ac[3] != NULL) erreur("[DHT] Indice table de Huffman AC doit valoir 0 ou 1");

   // Section SOS
   for (int i=0; i<3; i++) {
      if (img->comps->comps[i] != NULL) {
         // En baseline, les indices des tables de Huffman doivent être 0 ou 1
         if (img->comps->comps[i]->idhdc > 1) erreur("[SOS] Indice table de Huffman DC doit valoir 0 ou 1 (Baseline)");
         if (img->comps->comps[i]->idhac > 1) erreur("[SOS] Indice table de Huffman AC doit valoir 0 ou 1 (Baseline)");
      }
   }
   if (img->other->ss != 0) erreur("[SOS] Ss doit valoir 0 (Baseline)");
   if (img->other->se != 63) erreur("[SOS] Se doit valoir 63 (Baseline)");
   if (img->other->ah != 0) erreur("[SOS] Ah doit valoir 0 (Baseline)");
   if (img->other->al != 0) erreur("[SOS] Al doit valoir 0 (Baseline)");
}


static void verif_entete_progressif(img_t *img) {
   // Section SOF2
   // Précision 12 non traitée
   if (img->comps->precision_comp != 8) erreur("[SOF2] Précision composante : %d non pris charge (Progressif)", img->comps->precision_comp);
   for (int i=0; i<3; i++) {
      if (img->comps->comps[i] != NULL) {
         // En progressif, les indices des composantes doivent être entre 1 et 4
         if (img->comps->comps[i]->idc > 4) erreur("[SOF2] Indice composante doit valoir entre 1 et 4 (Progressif)");
      }
   }

   // Section SOS
   if (img->other->ss > 63) erreur("[SOS] Ss doit valoir entre 0 et 63 (Progressif)");
   if (img->other->se < img->other->ss || img->other->se > 63) erreur("[SOS] Se doit valoir entre Ss et 63 (Progressif)");
   // Ah != 0 non traité dans spectral selection
   if (img->other->ah != 0) erreur("[SOS] Ah doit valoir 0 (Progressif : spectral selection)");
   // Al != 0 non traité dans spectral selection
   if (img->other->al != 0) erreur("[SOS] Al doit valoir 0 (Progressif : spectral selection)");
}


static void calcul_image_information(img_t *img) {
   // Nombre de bloc horizontalement et verticalement
   // (Plus petit que le vrai nombre car ne prend pas en compte les MCU)
   int faux_nb_bloc_H = ceil((float)img->width / 8);
   int faux_nb_bloc_V = ceil((float)img->height / 8);

   // Calcul du hsampling et vsampling maximal
   uint8_t max_hsampling = 0;
   uint8_t max_vsampling = 0;
   for (int i=0; i<img->comps->nb; i++) {
      if (img->comps->comps[i]->hsampling > max_hsampling) max_hsampling = img->comps->comps[i]->hsampling;
      if (img->comps->comps[i]->vsampling > max_vsampling) max_vsampling = img->comps->comps[i]->vsampling;
   }
   img->max_vsampling = max_vsampling;
   img->max_hsampling = max_hsampling;
  
   // Nombre de MCU horizontalement et verticalement
   img->nbmcuH = ceil((float)faux_nb_bloc_H / max_hsampling);
   img->nbmcuV = ceil((float)faux_nb_bloc_V / max_vsampling);
   // Nombre total de MCU
   img->nbMCU = img->nbmcuH * img->nbmcuV;
}


img_t* init_img() {
   img_t *img   = calloc(1,sizeof(img_t));
   img->htables = calloc(1,sizeof(htables_t));
   img->comps   = calloc(1,sizeof(comps_t));
   img->section = calloc(1,sizeof(section_done_t));
   img->other   = calloc(1,sizeof(other_t));
   return img;
}


void decode_entete(FILE *fichier, bool premier_passage, img_t *img) {
   // On passe plusieurs fois dans cette fonction s'il y a plusieurs sections SOS dans l'image (Progressif)
   if (premier_passage) {
      // On vérifie que la section SOI est présente au début du fichier
      soi(fichier);
   }
   else {
      // On remet sos_done à faux dans le cas où il y a plusieurs sections SOS (mode progressif)
      img->section->sos_done = false;
   }

   // On boucle sur les marqueurs tant qu'on a pas atteint un SOS ou la fin de l'image
   while (!img->section->sos_done && !img->section->eoi_done && !fichier_fini(fichier)) {
      // Décodage des marqueurs
      marqueur(fichier, img);
   }

   if (img->section->sos_done) {
      // On effectue les calculs et les vérifications nécessaires si c'est le premier passage dans decode_entete
      if (premier_passage) {
         calcul_image_information(img);

         // Vérification des valeurs de l'entête
         verif_entete_app0(img);
         // Vérification pour le mode baseline
         if (img->section->num_sof == 0) verif_entete_baseline(img);
      }
      // Vérification pour le mode progressif
      if (img->section->num_sof == 2) verif_entete_progressif(img);
   }
   else {
      if (img->section->eoi_done) {
         // Si on a atteint un EOI avant un SOS dans le premier passage
         if (premier_passage) erreur("Image sans image");
      }
      else {
         // On affiche une erreur si on a atteint la fin du fichier avant une section SOS ou EOI
         erreur("L'image se termine sans EOI");
      }
   }
}


static void soi(FILE *fichier) {
   uint16_t marqueur = ((uint16_t)fgetc(fichier) << 8) + fgetc(fichier);
   if (marqueur != (uint16_t) 0xffd8) erreur("L'image doit commencer par 0xffd8 (SOI)");
}


static void marqueur(FILE *fichier, img_t *img) {
   // On lit un marqueur
   uint8_t marqueur[2];
   (void) !fread(&marqueur, 1, 2, fichier);

   // On vérifie que le marqueur commence par 0xff
   if (marqueur[0] != (uint8_t) 0xff) erreur("Octet n°%ld (%x) devrait être un marqueur %x %x", ftell(fichier)-2, ftell(fichier)-2, marqueur[0], marqueur[1]);
    
   // On associe le marqueur à la bonne section
   switch (marqueur[1]) {
      case (uint8_t) 0xc0:   // Section SOF0 (Baseline)
         img->section->num_sof = 0;
         sof(fichier, img);
         break;
      case (uint8_t) 0xc2:   // Section SOF2 (Progressif)
         img->section->num_sof = 2;
         sof(fichier, img);
         break;
      case (uint8_t) 0xc4:   // Section DHT
         dht(fichier, img);
         break;
      case (uint8_t) 0xd8:   // Section SOI
         // On a déja décodé une section SOI au début du fichier
         // Si on trouve une section SOI ici, on retourne une erreur
         erreur("Plusieurs SOI");
         break;
      case (uint8_t) 0xd9:   // Section EOI
         img->section->eoi_done = true;
         break;
      case (uint8_t) 0xda:   // Section SOS
         sos(fichier, img);
         break;
      case (uint8_t) 0xdb:   // Section DQT
         dqt(fichier, img);
         break;
      case (uint8_t) 0xe0:   // Section APP0
         app0(fichier, img);
         break;
      case (uint8_t) 0xfe:   // Section COMM
         com(fichier);
         break; 
      default: 
         erreur("Marqueur inconnu : %x", marqueur[1]);
   } 
}


static void app0(FILE *fichier, img_t *img) {
   // Vérification de la longueur de la section APP0
   // La section APP0 est obligatoirement de longueur 16
   uint16_t length = ((uint16_t)fgetc(fichier) << 8) + fgetc(fichier);
   if (length != 16) erreur("[APP0] Longueur section APP0 incorrect");

   // On récupère les informations pour les vérifier après
   (void) !fread(img->other->jfif, 1, 5, fichier);
   img->other->version_jfif_x = fgetc(fichier);
   img->other->version_jfif_y = fgetc(fichier);

   // On ignore les 7 prochains octets
   fseek(fichier, 7, SEEK_CUR);
   img->section->app0_done = true;
}


static void com(FILE *fichier) {
   // Vérification de la longueur de la section COM
   uint16_t length = ((uint16_t)fgetc(fichier) << 8) + fgetc(fichier);
   if (length < 2) erreur("[COM] Longueur section COM incorrect");

   // On ignore les commentaires
   fseek(fichier, length-2, SEEK_CUR);
}


static void sof(FILE *fichier, img_t *img) {
   // Il ne peut y avoir qu'une seule section SOF
   if (img->section->sof_done) erreur("[SOF] Plusieurs SOF");

   // On récupère les informations de la section SOF
   uint16_t length = ((uint16_t)fgetc(fichier) << 8) + fgetc(fichier);
   img->comps->precision_comp = fgetc(fichier);
   img->height = ((uint16_t)fgetc(fichier) << 8) + fgetc(fichier);
   img->width = ((uint16_t)fgetc(fichier) << 8) + fgetc(fichier);
   uint8_t nb_comp = fgetc(fichier);

   // Vérification de la longueur de la section SOF
   if (length != 8+3*nb_comp) erreur("[SOF] Longueur section SOF incorrect");

   // On associe chaque composante à ses facteurs d'échantillonnage et à sa table de quantification
   img->comps->nb = nb_comp;
   for (int i=0; i<=nb_comp-1; i++) {
      uint8_t id_comp = fgetc(fichier);
      if (id_comp == 0) erreur("[SOF] Indice composante doit être différent de 0");
      uint8_t sampling = fgetc(fichier);
      uint8_t id_quant = fgetc(fichier);
      img->comps->comps[i] = malloc(sizeof(idcomp_t));
      img->comps->comps[i]->idc = id_comp;
      img->comps->comps[i]->hsampling = sampling >> 4;
      img->comps->comps[i]->vsampling = sampling & 0b1111;
      img->comps->comps[i]->idq = id_quant;
   }
   // On indique qu'on a traité la section SOF
   img->section->sof_done = true;
}


static void dqt(FILE *fichier, img_t *img) {
   // Vérification de la longueur de la section DQT
   uint16_t length = ((uint16_t)fgetc(fichier) << 8) + fgetc(fichier);
   if ((length-2) % 65 != 0) erreur("[DQT] Longueur section DQT incorrect");

   // On boucle s'il y a plusieurs tables de quantification définies dans la section
   for (uint8_t n=1; n<=(length-2)/65; n++) {
      uint8_t octet = fgetc(fichier);

      // Vérification de la précision des tables de quantification
      // En mode baseline, il faut une précision de 8 bits
      uint8_t precision = octet >> 4;
      if (precision != 0 && precision != 1) erreur("[DQT] Précision table de quantification doit valoir 0 ou 1 (8 bits ou 16 bits)");
        
      uint8_t id_quant = octet & 0b1111;
      // On vérifie que l'indice de la table est entre 0 et 3
      if (id_quant > 3) erreur("[DQT] Indice table de quantification doit être entre 0 et 3");
        
      // Si la table n'existe pas, on la crée, sinon on la redéfinit
      if (img->qtables[id_quant] == NULL) {
         img->qtables[id_quant] = malloc(sizeof(qtable_prec_t));
         img->qtables[id_quant]->qtable = malloc(sizeof(qtable_t));
      }
      img->qtables[id_quant]->precision = precision;
      for (uint8_t i=0; i<64; i++) {
         if (precision == 0) {
            img->qtables[id_quant]->qtable->data[i] = fgetc(fichier);
         }
         else {
            img->qtables[id_quant]->qtable->data[i] = ((uint16_t)fgetc(fichier) << 8) + fgetc(fichier);
         }
      }
   }
   // On indique qu'on a traité la section DQT
   img->section->dqt_done = true;
}


static void remplir_huffman(huffman_tree_t *htable, uint16_t nb_symb, uint8_t lengths[nb_symb], uint8_t symb[nb_symb]) {
   // On crée une file pour stocker les noeuds de l'arbre de Huffman
   file_t *file = init_file();
   couple_tree_depth_t* couple = malloc(sizeof(couple_tree_depth_t));
   couple->tree = htable;
   couple->depth = 0;
    
   // On met le premier noeud dans la file
   insertion_file(file, couple);

   huffman_tree_t *tree;
   uint8_t depth;
   uint8_t num = 0;
   // Parcours en largeur de l'arbre pour le remplir
   while (num < nb_symb && !file_vide(file)) {

      // On récupère le noeud suivant de la file
      couple_tree_depth_t *couple = extraction_file(file);
      tree = couple->tree;
      depth = couple->depth;
      free(couple);

      // Si on atteint la bonne profondeur (longueur du code courant) on rempli l'arbre avec le symbole courant
      if (depth == lengths[num]) {
         tree->symb = symb[num];
         num++;
      }
      // Sinon on crée les fils gauche et droit et on augmente la profondeur
      else {
         tree->fils[0] = calloc(1,sizeof(huffman_tree_t));
         tree->fils[1] = calloc(1,sizeof(huffman_tree_t));
         couple_tree_depth_t *couple1 = malloc(sizeof(couple_tree_depth_t));
         couple_tree_depth_t *couple2 = malloc(sizeof(couple_tree_depth_t));
         couple1->tree = tree->fils[0];
         couple2->tree = tree->fils[1];
         couple1->depth = depth + 1;
         couple2->depth = depth + 1;

         // On met les fils dans la file
         insertion_file(file, couple1);
         insertion_file(file, couple2);
      }
   }
   // On libère la file
   if (file_vide(file)) erreur("[DHT] Code Huffman incorrect");
   while (!file_vide(file)) {
      couple_tree_depth_t *couple = extraction_file(file);
      free(couple);
   }
   free(file);
}


static void dht(FILE *fichier, img_t *img) {
   uint64_t debut = ftell(fichier);
   uint16_t length = ((uint16_t)fgetc(fichier) << 8) + fgetc(fichier);

   // On boucle s'il y a plusieurs tables de Huffman définies dans la section
   while ((uint64_t) ftell(fichier) < debut+length) {
      uint8_t octet = fgetc(fichier);

      // On vérifie que les 3 bits premiers sont 0
      if ((octet & 0b11100000) != 0) erreur("[DHT] 3 premiers bits de la section DHT doivent valoir 0");
	    
      // On récupère le type de la table (true pour DC, false pour AC)
      bool is_dc = (octet & 0b00010000) == 0;

      // On vérifie que l'indice de la table est entre 0 et 3
      uint8_t id_huff = octet & 0b1111;
      if (id_huff > 3) erreur("[DHT] Indice table de Huffman doit être entre 0 et 3");
        
      // On récupère les longueurs des codes
      uint8_t longueur_codes_brutes[16];
      (void) !fread(longueur_codes_brutes, 1, 16, fichier);

      // On compte le nombre de codes
      uint16_t nb_codes = 0;
      for (uint8_t i=0; i<16; i++) {
         nb_codes += longueur_codes_brutes[i];
      }

      // On vérifie qu'on a moins de 256 codes
      if (nb_codes > 256) erreur("[DHT] Plus de 256 symboles dans la table de Huffman");
        
      // On ordonne les longueurs codes par ordre croissant
      uint8_t longueur_codes_formatees[nb_codes];
      uint8_t i=0;
      for (uint8_t longueur=1; longueur<=16; longueur++) {
         for (uint16_t nb_longueur=0; nb_longueur<longueur_codes_brutes[longueur-1]; nb_longueur++) {
            longueur_codes_formatees[i] = longueur;
            i++;
         }
      }
      // On récupère les symboles
      uint8_t symb[nb_codes];
      (void) !fread(&symb, 1, nb_codes, fichier);

      // On crée la table de Huffman
      huffman_tree_t **htables;
      if (is_dc) htables = img->htables->dc;
      else htables = img->htables->ac;
      // Si la table existe déja, on la redéfinit
      if (htables[id_huff] != NULL) free_huffman_tree(htables[id_huff]);
      htables[id_huff] = calloc(1,sizeof(huffman_tree_t));

      // On remplit la table de Huffman sous forme d'un arbre
      remplir_huffman(htables[id_huff], nb_codes, longueur_codes_formatees, symb);
   }
   // On vérifie que la longueur de la section DHT correspond bien à ce qu'on a lu
   if ((uint64_t) ftell(fichier) != debut+length) erreur("[DHT] Longueur section DHT incorrect");
    
   // On indique qu'on a traité la section DHT
   img->section->dht_done = true;
}


static void verif_sos(img_t *img) {
   // On vérifie que chaque section a été traité avant de décoder SOS
   if (!img->section->app0_done) erreur("Image sans APP0 (ou SOS avant APP0)");
   if (!img->section->sof_done) erreur("Image sans SOF (ou SOS avant SOF)");
   if (!img->section->dqt_done) erreur("Image sans DQT (ou SOS avant DQT)");
   if (!img->section->dht_done) erreur("Image sans DHT (ou SOS avant DHT)");
}


static void sos(FILE *fichier, img_t *img) {
   // On vérifie qu'on a traité les autres sections
   verif_sos(img);

   uint16_t length = ((uint16_t)fgetc(fichier) << 8) + fgetc(fichier);

   // On vérifie que le nombre de composantes est bien le même que celui défini dans SOF
   uint8_t nb_comp = fgetc(fichier);
   if (nb_comp > img->comps->nb) erreur("[SOS] Nombre de composantes dans le SOS supérieur au nombre total");
    
   // Vérification de la longueur de la section SOS
   if (length != 6+2*nb_comp) erreur("[SOS] Longueur section SOS incorrect");

   // Réinitialise l'ordre des composantes
   for (int i=0; i<3; i++) {
      img->comps->ordre[i] = 0;
   }
   
   for (uint8_t i=0; i<=nb_comp-1; i++) {
      uint8_t id_comp = fgetc(fichier);
      uint8_t id_huff = fgetc(fichier);

      // On écrit l'ordre des composantes car elles ne sont pas forcément dans l'ordre Y Cb Cr
      img->comps->ordre[i] = id_comp;
        
      // On associe chaque composante à ses tables de Huffman
      uint8_t j = 0;
      while (img->comps->comps[j]->idc != id_comp && j <= 3) {j++;} // attention si id_comp pas dans comps
      if (j >= 4) erreur("[SOS] Indice de composante incorrect");
      img->comps->comps[j]->idhdc = id_huff >> 4;
      img->comps->comps[j]->idhac = id_huff & 0b1111;
   }
   // On récupère les informations pour les vérifier après
   img->other->ss = fgetc(fichier);
   img->other->se = fgetc(fichier);
   uint8_t a = fgetc(fichier);
   img->other->ah = a >> 4;
   img->other->al = a & 0b1111;

   // On indique qu'on a traité la section SOS
   img->section->sos_done = true;
}
