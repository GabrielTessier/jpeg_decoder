#include <upsampler.h>

// Retourne max(<a>, <b>, <c>)
int max(int a, int b, int c) {
  int t0 = (b>c)?b:c;
  return (a>t0)?a:t0;
}

int main(int argc, char *argv[]) {
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
    int nb_composantes;
    // déclaration image
    img_t img;
    img.comps->nb = nb_composantes;
    img.nbmcuH = 10;
    img.nbmcuV = 7;
    img.max_hsampling = max(h1, h2, h3);
    img.max_vsampling = max(v1, v2, v3);
    // composantes
    for (int j=0; j<nb_composantes; j++) {
      idcomp_t *idcomp = (idcomp_t *) malloc(sizeof(idcomp_t));
      idcomp.hsampling = subsampling_table[i][2*j];
      idcomp.vsampling = subsampling_table[i][2*j+1];
      img.comps->comps[j] = idcomp;
    }
    bloctu8_t ***ycc = (bloctu8_t ***) malloc(sizeof(bloctu8_t)*nb_composantes);

    for (int j=0; j<nb_composantes; j++) {
      int nb_blocs = (img.nbmcuH*img.comps->comps[j]->hsampling) * (img.nbmcuV*img.comps->comps[j]->vsampling);
        ycc[j] = (bloctu8_t **) malloc(sizeof(bloctu8_t *)*
        }
  }
}
