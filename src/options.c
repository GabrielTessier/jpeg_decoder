
#include "entete.h"
#include "options.h"

void set_verbose_param() { verbose = 1; }

void set_timer_param() { print_time = 1; }

void set_outfile(char* file) {
  if (outfile != NULL) erreur("Maximum une image en output.");
  outfile = file;
}

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

void set_option(const int argc, char **argv) {

  verbose = 0;
  print_time = 0;
  filepath = NULL;
  outfile = NULL;
  
  
  for (int i=1; i<argc; i++) {
    char* str = argv[i];
    if (str[0] != '-') {
      if (filepath != NULL) erreur("Deux images passées en paramètre.");
      filepath = str;
    } else {
      if (strlen(str) == 1) erreur("Pas d'option \"-\".");
      if (str[1] == '-') {	// option longue
	char* op = str+2;
	bool find = false;
	for (size_t p=0; p<sizeof(OPTION)/sizeof(option_t); p++) {
	  if (strcmp(op, OPTION[p].longname) == 0) {
	    OPTION[p].fnc();
	    find = true;
	    break;
	  }
	}
	if (!find) {
	  for (size_t p=0; p<sizeof(OPTION_PARAMETRE)/sizeof(option_t); p++) {
	    size_t size = strlen(OPTION_PARAMETRE[p].longname);
	    if (strncmp(op, OPTION_PARAMETRE[p].longname, size) == 0) {
	      if (op[size] == '=') {
		if (strlen(op+size+1) == 0) erreur("Manque la valeur pour le paramètre '%s'", OPTION_PARAMETRE[p].longname);
		OPTION_PARAMETRE[p].fnc(op+size+1);
		find = true;
		break;
	      }
	    }
	  }
	}
	if (!find) erreur("Pas de paramètre '%s'", op);
      } else {			// option courte
	char* oplist = str+1;
	size_t nbparam = strlen(oplist);
	for (size_t j=0; j<nbparam; j++) {
	  char op[2] = {oplist[j], 0};
	  bool find = false;
	  for (size_t p=0; p<sizeof(OPTION)/sizeof(option_t); p++) {
	    if (strcmp(op, OPTION[p].shortname) == 0) {
	      OPTION[p].fnc();
	      find = true;
	      break;
	    }
	  }
	  if (!find) {
	    for (size_t p=0; p<sizeof(OPTION_PARAMETRE)/sizeof(option_t); p++) {
	      if (strcmp(op, OPTION_PARAMETRE[p].shortname) == 0) {
		if (j != nbparam-1) erreur("Le paramètre '%c' ne peut pas avoir un paramètre collé derrière", op[0]);
		OPTION_PARAMETRE[p].fnc(argv[i+1]);
		i++;
		find = true;
		break;
	      }
	    }
	  }
	  if (!find) erreur("Pas de paramètre '%c'", op[0]);
	}
      }
    }
  }
}
