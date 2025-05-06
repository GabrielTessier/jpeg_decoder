#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "iqzz.h"
#include "vld.h"


struct idqtable_s {
    uint8_t id;
    qtable_t qtable;
};
typedef struct idqtable_s idqtable_t;

struct qtables_s {
    uint8_t nb;
    idqtable_t *qtables;
};
typedef struct qtables_s qtables_t;

struct idhtables_s {
    uint8_t id;
    huffman_tree_t htable;
};
typedef struct idhtables_s idhtables_t;

struct htables_s {
    uint8_t nb;
    idhtables_t *htables;
};
typedef struct htables_s htables_t;

struct idcomp_s {
    uint8_t idc;
    uint8_t idh;
    uint8_t idq;
};
typedef struct idcomp_s idcomp_t;

struct comps_s {
    uint8_t nb;
    idcomp_t *comps;
};
typedef struct comps_s comps_t;

struct img_s {
    uint16_t height;
    uint16_t width;
    qtables_t qtables;
    htables_t htables;
    comps_t comps;
};
typedef struct img_s img_t;

