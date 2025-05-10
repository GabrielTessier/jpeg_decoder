#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <utils.h>
#include <options.h>

enum option_type_e {LONGUE, COURTE};

struct poption_s{
  char* shortname;
  char* longname;
  void (*fnc)(all_option_t*, char*);
  char* param_name;
  char* description;
};
typedef struct poption_s poption_t;
struct option_s{
  char* shortname;
  char* longname;
  void (*fnc)(all_option_t*);
  char* description;
};
typedef struct option_s option_t;

static void set_verbose_param(all_option_t *all_option);
static void set_timer_param(all_option_t *all_option);
static void set_idct_fast_param(all_option_t *all_option);
static void set_outfile(all_option_t *all_option, char *file);
static char **get_max_size_str();
static bool try_apply_option(all_option_t *all_option, char *name,
                             enum option_type_e type);
static bool try_apply_poption(all_option_t *all_option, char *name, char *next,
                              enum option_type_e type);

static const option_t OPTION[4] = {
    {"v", "verbose", set_verbose_param, "Affiche des informations suplémentaire durant l'exécution."},
    {"t", "timer", set_timer_param, "Affiche le temps d'exécution de chaque partie."},
    {"h", "help", print_help, "Affiche cette aide."},
    {"f", "fast-idct", set_idct_fast_param, "Fast IDCT."}};

static const poption_t OPTION_PARAMETRE[1] = {
    {"o", "outfile", set_outfile, "fichier", "Placer la sortie dans le fichier."}};

static void set_verbose_param(all_option_t *all_option) { all_option->verbose = 1; }

static void set_timer_param(all_option_t *all_option) { all_option->print_time = 1; }

static void set_idct_fast_param(all_option_t *all_option) { all_option->idct_fast = 1; }

static void set_outfile(all_option_t *all_option, char* file) {
  if (all_option->outfile != NULL) erreur("Maximum une image en output.");
  all_option->outfile = file;
}

static char** get_max_size_str() {
  char** res = (char**) malloc(sizeof(char*)*2);
  int max_s = 2;
  int max_l = 0;
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

void print_help(all_option_t *all_option) {
  printf("Usage : %s [option] fichier\n", all_option->execname);
  printf("Option : \n");
  char **max_size = get_max_size_str();
  char *max_size_short = max_size[0];
  char *max_size_long = max_size[1];
  for (size_t i=0; i<sizeof(OPTION)/sizeof(option_t); i++) {
    printf("  ");
    if (OPTION[i].shortname != NULL) printf("-%c%s  ", OPTION[i].shortname[0], max_size_short+2);
    if (OPTION[i].longname != NULL) printf("--%s%s  ", OPTION[i].longname, max_size_long+2+strlen(OPTION[i].longname));
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
  exit(EXIT_SUCCESS);
}

static bool try_apply_option(all_option_t *all_option, char* name, enum option_type_e type) {
  for (size_t p=0; p<sizeof(OPTION)/sizeof(option_t); p++) {
    bool ok = (type == LONGUE && strcmp(name, OPTION[p].longname) == 0) || (type == COURTE && name[0] == OPTION[p].shortname[0]);
    if (ok) {
      OPTION[p].fnc(all_option);
      return true;
    }
  }
  return false;
}

static bool try_apply_poption(all_option_t *all_option, char* name, char* next, enum option_type_e type) {
  for (size_t p=0; p<sizeof(OPTION_PARAMETRE)/sizeof(poption_t); p++) {
    if (type == COURTE) {
      if (name[0] == OPTION_PARAMETRE[p].shortname[0]) {
	if (next == NULL) erreur("Manque la valeur pour le paramètre '%c'", name[0]);
	OPTION_PARAMETRE[p].fnc(all_option, next);
	return true;
      }
    } else {
      size_t size = strlen(OPTION_PARAMETRE[p].longname);
      if (strncmp(name, OPTION_PARAMETRE[p].longname, size) == 0) {
	if (name[size] == '=') {
	  if (strlen(name+size+1) == 0) erreur("Manque la valeur pour le paramètre '%s'", OPTION_PARAMETRE[p].longname);
	  OPTION_PARAMETRE[p].fnc(all_option, name+size+1);
	  return true;
	}
      }
    }
  }
  return false;
}

void set_option(all_option_t *all_option, const int argc, char **argv) {
  for (int i=1; i<argc; i++) {
    char* str = argv[i];
    if (str[0] != '-') {
      if (all_option->filepath != NULL) erreur("Deux images passées en paramètre.");
      all_option->filepath = str;
    } else {
      if (strlen(str) == 1) erreur("Pas d'option \"-\".");
      if (str[1] == '-') {	// option longue
	char* op = str+2;
	bool find = try_apply_option(all_option, op, LONGUE) || try_apply_poption(all_option, op, NULL, LONGUE);
	if (!find) erreur("Pas de paramètre '%s'", op);
      } else {			// option courte
	char* oplist = str+1;
	size_t nbparam = strlen(oplist);
	for (size_t j=0; j<nbparam; j++) {
	  char op[2] = {oplist[j], 0};
	  char* next = (i+1 < argc)?argv[i+1]:NULL;
	  bool find = try_apply_option(all_option, op, COURTE);
	  if (!find) {
	    find = try_apply_poption(all_option, op, next, COURTE);
	    if (find) {
	      if (j == nbparam-1) i++;
	      else erreur("Le paramètre '%c' ne peut pas avoir un paramètre collé derrière", op[0]);
	    }
	  }
	  if (!find) erreur("Pas de paramètre '%c'", op[0]);
	}
      }
    }
  }
}
