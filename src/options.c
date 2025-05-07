
#include "entete.h"
#include "options.h"

void set_verbose_param() { verbose = 1; }

void set_timer_param() { print_time = 1; }

void set_outfile(char* file) {
  if (outfile != NULL) erreur("Maximum une image en output.");
  outfile = file;
}

enum option_type_e {LONGUE, COURTE};

struct poption_s{
  char* shortname;
  char* longname;
  void (*fnc)(char*);
};
typedef struct poption_s poption_t;
struct option_s{
  char* shortname;
  char* longname;
  void (*fnc)(void);
};
typedef struct option_s option_t;

static const option_t OPTION[2] = {{"v", "verbose", set_verbose_param}, {"t", "timer", set_timer_param}};
static const poption_t OPTION_PARAMETRE[1] = {{"o", "outfile", set_outfile}};

bool try_apply_option(char* name, enum option_type_e type) {
  for (size_t p=0; p<sizeof(OPTION)/sizeof(option_t); p++) {
    bool ok = (type == LONGUE && strcmp(name, OPTION[p].longname) == 0) || (type == COURTE && name[0] == OPTION[p].shortname[0]);
    if (ok) {
      OPTION[p].fnc();
      return true;
    }
  }
  return false;
}

bool try_apply_poption(char* name, char* next, enum option_type_e type) {
  for (size_t p=0; p<sizeof(OPTION_PARAMETRE)/sizeof(option_t); p++) {
    if (type == COURTE) {
      if (name[0] == OPTION_PARAMETRE[p].shortname[0]) {
	if (next == NULL) erreur("Manque la valeur pour le paramètre '%c'", name[0]);
	OPTION_PARAMETRE[p].fnc(next);
	return true;
      }
    } else {
      size_t size = strlen(OPTION_PARAMETRE[p].longname);
      if (strncmp(name, OPTION_PARAMETRE[p].longname, size) == 0) {
	if (name[size] == '=') {
	  if (strlen(name+size+1) == 0) erreur("Manque la valeur pour le paramètre '%s'", OPTION_PARAMETRE[p].longname);
	  OPTION_PARAMETRE[p].fnc(name+size+1);
	  return true;
	}
      }
    }
  }
  return false;
}

void set_option(const int argc, char **argv) {
  for (int i=1; i<argc; i++) {
    char* str = argv[i];
    if (str[0] != '-') {
      if (filepath != NULL) erreur("Deux images passées en paramètre.");
      filepath = str;
    } else {
      if (strlen(str) == 1) erreur("Pas d'option \"-\".");
      if (str[1] == '-') {	// option longue
	char* op = str+2;
	bool find = try_apply_option(op, LONGUE) || try_apply_poption(op, NULL, LONGUE);
	if (!find) erreur("Pas de paramètre '%s'", op);
      } else {			// option courte
	char* oplist = str+1;
	size_t nbparam = strlen(oplist);
	for (size_t j=0; j<nbparam; j++) {
	  char op[2] = {oplist[j], 0};
	  char* next = (i+1 < argc)?argv[i+1]:NULL;
	  bool find = try_apply_option(op, COURTE);
	  if (!find) {
	    find = try_apply_poption(op, next, COURTE);
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
