#include <stdlib.h>

#include "test_utils.h"
#include <iqzz.h>

int main(int argc, char* argv[]) {
  // pour éviter une erreur à la compilation
  (void) argc;
  // test zig-zag inverse
  blocl16_t line;
  for (int i=0; i<64; i++) {
    line.data[i] = i;
  }
  bloct16_t *res = izz(&line);
  
  int16_t temp[8][8] = {
    {0,  1,  5,  6,  14, 15, 27, 28},
    {2,  4,  7,  13, 16, 26, 29, 42},
    {3,  8,  12, 17, 25, 30, 41, 43},
    {9,  11, 18, 24, 31, 40, 44, 53},
    {10, 19, 23, 32, 39, 45, 52, 54},
    {20, 22, 33, 38, 46, 51, 55, 60},
    {21, 34, 37, 47, 50, 56, 59, 61},
    {35, 36, 48, 49, 57, 58, 62, 63}};
  bloct16_t ref;  
  for (int i=0; i<8; i++) {
    for (int j=0; j<8; j++) {
      ref.data[i][j] = temp[i][j];
    }
  }
  int test_izz = 1;
  for (int i=0; i<8; i++) {
    for (int j=0; j<8; j++) {
      if (res->data[j][i] != ref.data[i][j]) {
        test_izz = 0;
      }
    }
  }
  free(res);
  test_res(test_izz, "zig-zag inverse", argv);
  
  // test quantification inverse
  blocl16_t line2;
  qtable_t qt;
  for (int i=0; i<64; i++) {
    line2.data[i] = line.data[i]*2;
    qt.data[i] = line.data[i];
  }
  blocl16_t *res2;
  res2 = iquant(&line2, &qt);
  // Vérification
  int test_iq = 1;
  for (int i=0; i<64; i++) {
    if (res2->data[i] != qt.data[i]*line2.data[i]) {
      test_iq = 0;
    }
  }
  free(res2);
  test_res(test_iq, "quantification inverse", argv);
  return 0;
}
