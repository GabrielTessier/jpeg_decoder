CC = gcc
LD = gcc

# -O0 désactive les optimisations à la compilation
# C'est utile pour débugger, par contre en "production"
# on active au moins les optimisations de niveau 2 (-O2).
CFLAGS_DEBUG = -Wall -Wextra -std=c99 -Iinclude -O0 -fsanitize=address,undefined -g -lm
LDFLAGS_DEBUG = -fsanitize=address,undefined -lm

CFLAGS_FAST = -std=c99 -Iinclude -Ofast -lm
LDFLAGS_FAST = -lm

CFLAGS_SANS_OPT = -std=c99 -Iinclude -O0 -lm
LDFLAGS_SANS_OPT = -lm

# Par défaut, on compile tous les fichiers source (.c) qui se trouvent dans le
# répertoire src/
SRC_FILES=$(wildcard src/*.c)

# Par défaut, la compilation de src/toto.c génère le fichier objet obj/toto.o
OBJ_FILES_DEBUG=$(patsubst src/%.c,obj/debug/%.o,$(SRC_FILES))
OBJ_FILES_FAST=$(patsubst src/%.c,obj/fast/%.o,$(SRC_FILES))
OBJ_FILES_SANS_OPT=$(patsubst src/%.c,obj/sans_opt/%.o,$(SRC_FILES))

all: makedir jpeg2ppm_sans_opt jpeg2ppm_debug jpeg2ppm_fast

makedir :
	mkdir -p obj/debug
	mkdir -p obj/fast
	mkdir -p obj/sans_opt

jpeg2ppm_sans_opt: $(OBJ_FILES_SANS_OPT)
	$(LD) $(OBJ_FILES_SANS_OPT) $(LDFLAGS_SANS_OPT) -o $@
	ln -s $@ jpeg2ppm

jpeg2ppm_debug: $(OBJ_FILES_DEBUG) 
	$(LD) $(OBJ_FILES_DEBUG) $(LDFLAGS_DEBUG) -o $@

jpeg2ppm_fast: $(OBJ_FILES_FAST) 
	$(LD) $(OBJ_FILES_FAST) $(LDFLAGS_FAST) -o $@

obj/debug/%.o: src/%.c
	$(CC) -c $(CFLAGS_DEBUG) $< -o $@

obj/fast/%.o: src/%.c
	$(CC) -c $(CFLAGS_FAST) $< -o $@

obj/sans_opt/%.o: src/%.c
	$(CC) -c $(CFLAGS_SANS_OPT) $< -o $@

.PHONY: clean

clean:
	rm -rf jpeg2ppm* obj/
