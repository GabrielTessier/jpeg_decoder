#pragma once

#include <stdint.h>


// Tableau 1x64 représentant un bloc 8x8 d'entiers 8 bits signés
struct blocl8_s {
  int8_t data[64];
};
typedef struct blocl8_s blocl8_t;

// Tableau 8x8 représentant un bloc 8x8 d'entiers 8 bits signés
struct bloct8_s {
  int8_t data[8][8];
};
typedef struct bloct8_s bloct8_t;

// Tableau 1x64 représentant un bloc 8x8 d'entiers 8 bits non signés
struct bloclu8_s {
  uint8_t data[64];
};
typedef struct bloclu8_s bloclu8_t;

// Tableau 8x8 représentant un bloc 8x8 d'entiers 8 bits non signés
struct bloctu8_s {
  uint8_t data[8][8];
};
typedef struct bloctu8_s bloctu8_t;

// Tableau 1x64 représentant un bloc 8x8 d'entiers 16 bits signés
struct blocl16_s {
  int16_t data[64];
};
typedef struct blocl16_s blocl16_t;

// Tableau 8x8 représentant un bloc 8x8 d'entiers 16 bits signés
struct bloct16_s {
  int16_t data[8][8];
};
typedef struct bloct16_s bloct16_t;
