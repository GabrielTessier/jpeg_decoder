#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "iqzz.h"


struct idqtable_s {
    uint8_t id;
    qtable_t qtable;
};

typedef struct idqtable_s idqtable_t;

struct qtables_s {
    idqtable_t *qtables;
    uint8_t nb;
};

typedef struct qtables_s qtables_t;

struct img_s {
    uint16_t height;
    uint16_t width;
    uint8_t nb_comp;
    qtables_t qtables;
};

