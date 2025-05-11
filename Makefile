CC = gcc
LD = gcc

# -O0 désactive les optimisations à la compilation
# C'est utile pour débugger, par contre en "production"
# on active au moins les optimisations de niveau 2 (-O2).
CFLAGS_DEBUG = -Wall -Wextra -std=c99 -Iinclude/ -O0 -fsanitize=address,undefined -g -lm
LDFLAGS_DEBUG = -fsanitize=address,undefined -lm

CFLAGS_FAST = -std=c99 -Iinclude/ -Ofast -lm
LDFLAGS_FAST = -lm

CFLAGS_SANS_OPT = -std=c99 -Iinclude/ -O0 -lm
LDFLAGS_SANS_OPT = -lm

# Par défaut, on compile tous les fichiers source (.c) qui se trouvent dans le
# répertoire src/
SRC_FILES=$(wildcard src/*.c)
TEST_FILES=$(wildcard test/*.c)

# Par défaut, la compilation de src/toto.c génère le fichier objet obj/toto.o
OBJ_FILES_DEBUG=$(patsubst src/%.c,obj/debug/%.o,$(SRC_FILES))
OBJ_FILES_FAST=$(patsubst src/%.c,obj/fast/%.o,$(SRC_FILES))
OBJ_FILES_SANS_OPT=$(patsubst src/%.c,obj/sans_opt/%.o,$(SRC_FILES))
OBJ_FILES_TEST=$(patsubst test/%.c, obj/test/%.o,$(TEST_FILES))$(patsubst test/%_test.c, obj/sans_opt/%.o,$(TEST_FILES)) obj/sans_opt/utils.o

all: jpeg2ppm_sans_opt jpeg2ppm_debug jpeg2ppm_fast

makedir :
	mkdir -p obj/debug
	mkdir -p obj/fast
	mkdir -p obj/sans_opt
	mkdir -p obj/test
	mkdir -p build

jpeg2ppm_sans_opt: makedir $(OBJ_FILES_SANS_OPT)
	$(LD) $(OBJ_FILES_SANS_OPT) $(LDFLAGS_SANS_OPT) -o $@
	rm -f jpeg2ppm
	ln -s $@ jpeg2ppm

jpeg2ppm_debug: makedir $(OBJ_FILES_DEBUG) 
	$(LD) $(OBJ_FILES_DEBUG) $(LDFLAGS_DEBUG) -o $@

jpeg2ppm_fast: makedir $(OBJ_FILES_FAST) 
	$(LD) $(OBJ_FILES_FAST) $(LDFLAGS_FAST) -o $@

test: makedir $(OBJ_FILES_TEST)
	$(LD) $(OBJ_FILES_TEST) $(LDFLAGS_SANS_OPT) -o build/iqzz_test

obj/debug/%.o: src/%.c
	$(CC) -c $(CFLAGS_DEBUG) $< -o $@

obj/fast/%.o: src/%.c
	$(CC) -c $(CFLAGS_FAST) $< -o $@

obj/sans_opt/%.o: src/%.c
	$(CC) -c $(CFLAGS_SANS_OPT) $< -o $@

obj/test/%.o: test/%.c src/iqzz.c
	$(CC) -c $(CFLAGS_SANS_OPT) $< -o $@

.PHONY: clean

clean:
	rm -rf jpeg2ppm* obj/ build/
