CC = gcc
LD = gcc

# -O0 désactive les optimisations à la compilation
# C'est utile pour débugger, par contre en "production"
# on active au moins les optimisations de niveau 2 (-O2).
CFLAGS_DEBUG= -Wall -Wextra -std=c99 -Iinclude/ -O0 -fsanitize=address,undefined -g -lm
LDFLAGS_DEBUG= -fsanitize=address,undefined -lm

CFLAGS_FAST = -std=c99 -Iinclude/ -Ofast -lm
LDFLAGS_FAST = -lm

CFLAGS_SANS_OPT = -std=c99 -Iinclude/ -O2 -lm 
LDFLAGS_SANS_OPT = -lm

CFLAGS_TEST=-Wall -Wextra -std=c99 -Iinclude/ -O0 -fsanitize=address,undefined -g -lm
LDFLAGS_TEST=-fsanitize=address,undefined -lm

CFLAGS_PROF = -std=c99 -Iinclude/ -lm -pg
LDFLAGS_PROF = -lm -pg

SRC_DIR=src
TEST_DIR=test
BIN_DIR=bin
OBJ_DIR=obj

SRC_FILES=$(wildcard $(SRC_DIR)/*.c)
TEST_FILES=$(patsubst %.c,%_test,$(shell cat $(TEST_DIR)/test.txt))
TEST_OPTION=debug

# Par défaut, la compilation de src/toto.c génère le fichier objet obj/toto.o
OBJ_FILES_DEBUG=$(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/debug/%.o,$(SRC_FILES))
OBJ_FILES_FAST=$(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/fast/%.o,$(SRC_FILES))
OBJ_FILES_PROF=$(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/sans_opt/%.o,$(SRC_FILES))
OBJ_FILES_SANS_OPT=$(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/sans_opt/%.o,$(SRC_FILES))



all: jpeg2ppm_sans_opt jpeg2ppm_debug jpeg2ppm_fast jpeg2ppm_prof test 

makedir :
	mkdir -p $(OBJ_DIR)/debug
	mkdir -p $(OBJ_DIR)/fast
	mkdir -p $(OBJ_DIR)/sans_opt
	mkdir -p $(OBJ_DIR)/test
	mkdir -p $(BIN_DIR)

jpeg2ppm_sans_opt: makedir $(OBJ_FILES_SANS_OPT)
	$(LD) $(OBJ_FILES_SANS_OPT) $(LDFLAGS_SANS_OPT) -o $(BIN_DIR)/$@
	ln -f -s $(BIN_DIR)/$@ jpeg2ppm

jpeg2ppm_debug: makedir $(OBJ_FILES_DEBUG) 
	$(LD) $(OBJ_FILES_DEBUG) $(LDFLAGS_DEBUG) -o $(BIN_DIR)/$@

jpeg2ppm_fast: makedir $(OBJ_FILES_FAST) 
	$(LD) $(OBJ_FILES_FAST) $(LDFLAGS_FAST) -o $(BIN_DIR)/$@

jpeg2ppm_prof: makedir $(OBJ_FILES_PROF) 
	$(LD) $(OBJ_FILES_PROF) $(LDFLAGS_PROF) -o $(BIN_DIR)/$@

test: makedir $(TEST_FILES)

test_run: test
	@echo
	@echo "---------- Début des tests ----------"
	@for exec in $(TEST_FILES); do \
		./$(BIN_DIR)/$$exec; \
	done

idct_opt_test: $(OBJ_DIR)/test/idct_opt_test.o $(OBJ_DIR)/$(TEST_OPTION)/idct_opt.o $(OBJ_DIR)/$(TEST_OPTION)/idct.o $(OBJ_DIR)/test/test_utils.o
	$(LD) $^ $(LDFLAGS_TEST) -o $(BIN_DIR)/$@

entete_test: $(OBJ_DIR)/test/entete_test.o $(OBJ_DIR)/$(TEST_OPTION)/entete.o $(OBJ_DIR)/$(TEST_OPTION)/img.o $(OBJ_DIR)/$(TEST_OPTION)/options.o $(OBJ_DIR)/$(TEST_OPTION)/file.o $(OBJ_DIR)/$(TEST_OPTION)/utils.o $(OBJ_DIR)/test/test_utils.o
	$(LD) $^ $(LDFLAGS_TEST) -o $(BIN_DIR)/$@

vld_test: $(OBJ_DIR)/test/vld_test.o $(OBJ_DIR)/$(TEST_OPTION)/vld.o $(OBJ_DIR)/$(TEST_OPTION)/utils.o $(OBJ_DIR)/$(TEST_OPTION)/options.o $(OBJ_DIR)/$(TEST_OPTION)/bitstream.o $(OBJ_DIR)/test/test_utils.o
	$(LD) $^ $(LDFLAGS_TEST) -o $(BIN_DIR)/$@

iqzz_test: $(OBJ_DIR)/test/iqzz_test.o $(OBJ_DIR)/$(TEST_OPTION)/iqzz.o $(OBJ_DIR)/test/test_utils.o 
	$(LD) $^ $(LDFLAGS_TEST) -o $(BIN_DIR)/$@

$(OBJ_DIR)/debug/%.o: src/%.c
	$(CC) -c $(CFLAGS_DEBUG) $< -o $@

$(OBJ_DIR)/fast/%.o: src/%.c
	$(CC) -c $(CFLAGS_FAST) $< -o $@

$(OBJ_DIR)/sans_opt/%.o: src/%.c
	$(CC) -c $(CFLAGS_SANS_OPT) $< -o $@

$(OBJ_DIR)/prof/%.o: src/%.c
	$(CC) -c $(CFLAGS_PROF) $< -o $@

$(OBJ_DIR)/test/%_test.o: test/%_test.c src/%.c
	$(CC) -c $(CFLAGS_TEST) $< -o $@

$(OBJ_DIR)/test/test_utils.o: test/test_utils.c
	$(CC) -c $(CFLAGS_TEST) $< -o $@

.PHONY: clean

clean:
	rm -rf $(BIN_DIR)/jpeg2ppm* $(OBJ_DIR)/ $(BIN_DIR)/*_test
