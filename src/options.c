#include "erreur.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <utils.h>
#include <options.h>

// Variable globale contenant toutes les informations sur les différentes
// options disponibles
all_option_t all_option;

// Énumération des types d'option disponibles
enum option_type_e {LONGUE, COURTE};

// Structure pour les options à paramètre
struct poption_s{
   char* shortname;
   char* longname;
   erreur_t (*fnc)(all_option_t*, char*);
   char* param_name;
   char* description;
};
typedef struct poption_s poption_t;

// Structure pour les options sans paramètre
struct option_s{
   char* shortname;
   char* longname;
   erreur_t (*fnc)(all_option_t*);
   char* description;
};
typedef struct option_s option_t;

// Active l'option verbose (des détails de l'exécution sont affichés sur la sortie standard)
static erreur_t set_verbose_param(all_option_t *all_option);
// Active l'option timer (les différentes parties de l'exécution sont chronométrées et affichées sur la sortie standard)
static erreur_t set_timer_param(all_option_t *all_option);
// Active l'option IDCT naïve
static erreur_t set_idct_nofast_param(all_option_t *all_option);
// Active l'option outfile (choix du fichier de sortie à <file>)
static erreur_t set_outfile(all_option_t *all_option, char *file);
// Affiche les tables de huffman et de quantification
static erreur_t print_tables(all_option_t *all_option);
// Active l'option pour afficher l'aide
static erreur_t set_print_help(all_option_t *all_option);
// Retourne un tableau 2x1 contenant la longueur maximale des noms de paramètre,
// et la longueur maximale des noms d'option suivi de leur paramètre
// (pour l'affichage de l'aide avec la fonction 'print_help)
static char **get_max_size_str();
static erreur_t try_apply_option(all_option_t *all_option, char *name, enum option_type_e type, bool *success);
static erreur_t try_apply_poption(all_option_t *all_option, char *name, char *next, enum option_type_e type, bool *opt_ok);



static const option_t OPTION[5] = {
   {"v", "verbose", set_verbose_param, "Affiche des informations supplémentaires durant l'exécution."},
   {"t", "timer", set_timer_param, "Affiche le temps d'exécution de chaque partie."},
   {"h", "help", set_print_help, "Affiche cette aide."},
   {"f", "no-fast-idct", set_idct_nofast_param, "N'utilise pas l'IDCT rapide."},
   {NULL, "tables", print_tables, "Affiche les tables de Huffman et de quantification"}};

static const poption_t OPTION_PARAMETRE[1] = {
   {"o", "outfile", set_outfile, "fichier", "Place la sortie dans le fichier."}};

static erreur_t set_verbose_param(all_option_t *all_option) {
   all_option->verbose = 1;
   return (erreur_t) {.code=SUCCESS};
}

static erreur_t set_timer_param(all_option_t *all_option) {
   all_option->print_time = 1;
   return (erreur_t){.code = SUCCESS};
}

static erreur_t set_idct_nofast_param(all_option_t *all_option) {
   all_option->idct_fast = 0;
   return (erreur_t){.code = SUCCESS};
}

static erreur_t set_outfile(all_option_t *all_option, char* file) {
  if (all_option->outfile != NULL) return (erreur_t) {.code=ERR_PARAM, .com="Maximum une image en output.", .must_free = false};
   all_option->outfile = file;
   return (erreur_t) {.code=SUCCESS};
}

static erreur_t print_tables(all_option_t *all_option) {
   all_option->print_tables = true;
   return (erreur_t) {.code = SUCCESS};
}

static erreur_t set_print_help(all_option_t *all_option) {
   all_option->print_help = true;
   return (erreur_t) {.code = SUCCESS};
}

static char** get_max_size_str() {
   char** res = (char**) malloc(sizeof(char*)*2);
   int max_s = 2; // longueur maximale des noms de paramètre
   int max_l = 0; // longueur maximale des noms d'option + nom de son paramètre
   for (size_t i=0; i<sizeof(OPTION_PARAMETRE)/sizeof(poption_t); i++) {
      int ps = 5+sizeof(OPTION_PARAMETRE[i].param_name);
      if (ps > max_s) max_s = ps;
      int pl = 5+sizeof(OPTION_PARAMETRE[i].longname)+sizeof(OPTION_PARAMETRE[i].param_name);
      if (pl > max_l) max_l = pl;
   }
   for (size_t i=0; i<sizeof(OPTION)/sizeof(option_t); i++) {
      int pl = 2+sizeof(OPTION[i].longname);
      if (pl > max_l) max_l = pl;
   }
   res[0] = (char*) malloc(sizeof(char)*(max_s+1));
   res[1] = (char*) malloc(sizeof(char)*(max_l+1));
   for (int i=0; i<max_s; i++) res[0][i] = ' ';
   for (int i=0; i<max_l; i++) res[1][i] = ' ';
   res[0][max_s] = 0;
   res[1][max_l] = 0;
   return res;
}

erreur_t print_help(all_option_t *all_option) {
   printf("Usage : %s [option] fichier\n", all_option->execname);
   printf("Option : \n");
   char **max_size = get_max_size_str();
   char *max_size_short = max_size[0];
   char *max_size_long = max_size[1];
   for (size_t i=0; i<sizeof(OPTION)/sizeof(option_t); i++) {
      printf("  ");
      if (OPTION[i].shortname != NULL) printf("-%c%s  ", OPTION[i].shortname[0], max_size_short+2);
      else printf("%s  ", max_size_short);
      if (OPTION[i].longname != NULL) printf("--%s%s  ", OPTION[i].longname, max_size_long+2+strlen(OPTION[i].longname));
      else printf("%s  ", max_size_long);
      if (OPTION[i].description != NULL) printf("%s", OPTION[i].description);
      printf("\n");
   }
   for (size_t i=0; i<sizeof(OPTION_PARAMETRE)/sizeof(poption_t); i++) {
      printf("  ");
      if (OPTION_PARAMETRE[i].shortname != NULL) printf("-%c <%s>%s  ", OPTION_PARAMETRE[i].shortname[0], OPTION_PARAMETRE[i].param_name, max_size_short+5+strlen(OPTION_PARAMETRE[i].param_name));
      if (OPTION_PARAMETRE[i].longname != NULL) printf("--%s=<%s>%s  ", OPTION_PARAMETRE[i].longname, OPTION_PARAMETRE[i].param_name, max_size_long+5+strlen(OPTION_PARAMETRE[i].longname)+strlen(OPTION_PARAMETRE[i].param_name));
      if (OPTION_PARAMETRE[i].description != NULL) printf("%s", OPTION_PARAMETRE[i].description);
      printf("\n");
   }
   free(max_size_short);
   free(max_size_long);
   free(max_size);
   return (erreur_t) {.code=SUCCESS};
}

static erreur_t try_apply_option(all_option_t *all_option, char* name, enum option_type_e type, bool *success) {
   for (size_t p=0; p<sizeof(OPTION)/sizeof(option_t); p++) {
      bool ok = (type == LONGUE && OPTION[p].longname != NULL && strcmp(name, OPTION[p].longname) == 0) || (type == COURTE && OPTION[p].shortname != NULL && name[0] == OPTION[p].shortname[0]);
      if (ok) {
	 erreur_t err = OPTION[p].fnc(all_option);
	 *success = true;
	 return err;
      }
   }
   *success = false;
   return (erreur_t) {.code = SUCCESS};
}

static erreur_t try_apply_poption(all_option_t *all_option, char* name, char* next, enum option_type_e type, bool *opt_ok) {
   for (size_t p=0; p<sizeof(OPTION_PARAMETRE)/sizeof(poption_t); p++) {
      if (type == COURTE && OPTION_PARAMETRE[p].shortname != NULL) {
	 if (name[0] == OPTION_PARAMETRE[p].shortname[0]) {
	    if (next == NULL) {
	       char *str = (char*) malloc(sizeof(char) * 40);
	       sprintf(str, "Manque la valeur pour le paramètre '%c'", name[0]);	       
	       return (erreur_t) {.code=ERR_OPT, .com=str, .must_free = true};
	    }
	    erreur_t err = OPTION_PARAMETRE[p].fnc(all_option, next);
	    *opt_ok = true;
	    return err;
	 }
      } else if (OPTION_PARAMETRE[p].longname != NULL) {
	 size_t size = strlen(OPTION_PARAMETRE[p].longname);
	 if (strncmp(name, OPTION_PARAMETRE[p].longname, size) == 0) {
	    if (name[size] == '=') {
	       if (strlen(name+size+1) == 0) {
		  char *str = malloc(sizeof(char) * (39 + strlen(OPTION_PARAMETRE[p].longname)));
		  sprintf(str, "Manque la valeur pour le paramètre '%s'", OPTION_PARAMETRE[p].longname);
		  return (erreur_t) {.code=ERR_PARAM, .com=str, .must_free = true};
	       }
	       erreur_t err = OPTION_PARAMETRE[p].fnc(all_option, name+size+1);
	       *opt_ok = true;
	       return err;
	    }
	 }
      }
   }
   *opt_ok = false;
   return (erreur_t) {.code = SUCCESS};
}

erreur_t set_option(all_option_t *all_option, const int argc, char **argv) {
   all_option->idct_fast = 1;
   for (int i=1; i<argc; i++) {
      char* str = argv[i];
      if (str[0] != '-') {
	 if (all_option->filepath != NULL) return (erreur_t) {.code=ERR_PARAM, .com="Deux images passées en paramètre.", .must_free = false};
	 all_option->filepath = str;
      } else {
	 if (strlen(str) == 1) return (erreur_t) {.code=ERR_OPT, .com="Pas d'option \"-\".", .must_free = false};
	 if (str[1] == '-') {	// option longue
	    char* op = str+2;
	    bool find, find2;
	    erreur_t err = try_apply_poption(all_option, op, NULL, LONGUE, &find);
	    if (err.code) return err;
	    err = try_apply_option(all_option, op, LONGUE, &find2);
	    if (err.code) return err;
	    find = find || find2;
	    if (!find) {
	       char *str = (char*) malloc(sizeof(char) * (20 + strlen(op)));
	       sprintf(str, "Pas de paramètre '%s'", op);
	       return (erreur_t) {.code=ERR_PARAM, .com=str, .must_free = true};
	    }
	 } else {		// option courte
	    char* oplist = str+1;
	    size_t nbparam = strlen(oplist);
	    for (size_t j=0; j<nbparam; j++) {
	       char op[2] = {oplist[j], 0};
	       char* next = (i+1 < argc)?argv[i+1]:NULL;
	       bool find;
	       erreur_t err = try_apply_option(all_option, op, COURTE, &find);
	       if (err.code) return err;
	       if (!find) {
		  erreur_t err = try_apply_poption(all_option, op, next, COURTE, &find);
		  if (err.code) return err;
		  if (find) {
		     if (j == nbparam-1) i++;
		     else {
			char *str = (char*) malloc(sizeof(char) * 67);
			sprintf(str, "Le paramètre '%c' ne peut pas avoir un paramètre collé derrière", op[0]);
			return (erreur_t) {.code=ERR_PARAM, .com=str, .must_free = true};
		     }
		  }
	       }
	       if (!find) {
		  char *str = (char*) malloc(sizeof(char) * 22);
		  sprintf(str, "Pas de paramètre '%c'", op[0]);
		  return (erreur_t) {.code=ERR_PARAM, .com=str, .must_free = true};
	       }
	    }
	 }
      }
   }
   return (erreur_t) {.code=SUCCESS};
}
