#include "test_utils.h"
#include <upsampler.h>

// Initialise une structure img
static img_t* init_img();

// Retourne max(<a>, <b>, <c>)
int max(int a, int b, int c) {
  int t0 = (b>c)?b:c;
  return (a>t0)?a:t0;
}

static img_t* init_img() {
    img_t *img = calloc(1,sizeof(img_t));
    img->htables = calloc(1,sizeof(htables_t));
    img->comps = calloc(1,sizeof(comps_t));
    img->section = calloc(1,sizeof(section_done_t));
    img->other = calloc(1,sizeof(other_t));
    return img;
}

int main(int argc, char *argv[]) {
  // pour éviter une erreur à la compilation
  (void) argc;
  // IMAGES NOIR ET BLANC 
  
  // IMAGES COULEUR
  // HY, VY, HCb, VCb, HCr, VCr
  int subsampling_table[12][6] = {
    {1, 1, 1, 1, 1, 1},
    {1, 2, 1, 1, 1, 1},
    {1, 4, 1, 1, 1, 1}, // irregular notation ?
    {1, 4, 1, 2, 1, 2},
    {2, 1, 1, 1, 1, 1},
    {2, 2, 1, 1, 1, 1},
    {2, 2, 2, 2, 1, 1},
    {2, 4, 1, 1, 1, 1}, // irregular notation ?
    {4, 1, 1, 1, 1, 1},
    {4, 1, 1, 2, 1, 2},
    {4, 2, 1, 1, 1, 1},
    {4, 2, 2, 2, 2, 2}
  };

  for (int i=0; i<12; i++) {
    int h1 = subsampling_table[i][0];
    int v1 = subsampling_table[i][1];
    int h2 = subsampling_table[i][2];
    int v2 = subsampling_table[i][3];
    int h3 = subsampling_table[i][4];
    int v3 = subsampling_table[i][5];
    int nb_composantes = 3;
    // déclaration image
    img_t *img = init_img();
    img->comps->nb = nb_composantes;
    img->nbmcuH = 10;
    img->nbmcuV = 7;
    img->max_hsampling = max(h1, h2, h3);
    img->max_vsampling = max(v1, v2, v3);
    // composantes
    for (int j=0; j<nb_composantes; j++) {
      idcomp_t *idcomp = (idcomp_t *) malloc(sizeof(idcomp_t));
      idcomp->hsampling = subsampling_table[i][2*j];
      idcomp->vsampling = subsampling_table[i][2*j+1];
      img->comps->comps[j] = idcomp;
    }
    bloctu8_t ***ycc = (bloctu8_t ***) malloc(sizeof(bloctu8_t **)*nb_composantes);

    for (int j=0; j<nb_composantes; j++) { // parcours des composantes
      uint8_t hsamp = img->comps->comps[j]->hsampling;
      uint8_t vsamp = img->comps->comps[j]->vsampling;
      /* float hfact = (float) h1/hsamp; */
      /* float vfact = (float) v1/vsamp; */
      int nb_blocs = (hsamp*img->nbmcuH) * (vsamp*img->nbmcuV); // TODO: prévoir un test tronqué
      ycc[j] = (bloctu8_t **) malloc(sizeof(bloctu8_t *)*nb_blocs);
      for (int k=0; k<nb_blocs; k++) { // parcours des blocs
        for (int l=0; l<8; l++) {
          for (int m=0; m<8; m++) {
            ycc[j][k]->data[l][m] = (k%hsamp) + ((k/hsamp)%vsamp)*hsamp;
          }
        }
      }
    }
    bloctu8_t ***ycc_up = upsampler(img, ycc);
    // bloctu8_t ***ycc_ref = (bloctu8_t ***) malloc(sizeof(bloctu8_t)*nb_composantes);
    int test_upsampler = 1;
    int nb_blocs = (h1*img->nbmcuH) * (v1*img->nbmcuV); // prévoir un test tronqué
    for (int j=0; j<nb_composantes; j++) {
      //ycc_ref[j] = (bloctu8_t **) malloc(sizeof(bloctu8_t *)*nb_blocs);
      for (int k=0; k<nb_blocs && test_upsampler; k++) {
        for (int l=0; l<8; l++) {
          for (int m=0; m<8; m++) {
            uint8_t ref = 0;
            if (ycc_up[j][k]->data[l][m] != ref) {
              test_upsampler = 0; // TODO
            }
          }
        }
      }
    }
    char test_name[80];
    sprintf(test_name, "HVs : %d %d %d %d %d %d\n", h1, v1, h2, v2, h3, v3);
    test_res(test_upsampler, test_name, argv);
  }
}
