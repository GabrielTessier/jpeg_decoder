
#include <ycc2rgb.h>
#include <decoder_utils.h>

void save_mcu_ligne(FILE *outputfile, img_t *img, bloctu8_t ***ycc, char *rgb, uint8_t yhf, uint8_t yvf, uint8_t y_id, uint64_t nb_blocYH, uint8_t cbhf, uint8_t cbvf, uint8_t cb_id, uint64_t nb_blocCbH, uint8_t crhf, uint8_t crvf, uint8_t cr_id, uint64_t nb_blocCrH) {
   const uint8_t nbcomp = img->comps->nb;
   for (uint64_t y = 0; y < img->max_vsampling * 8; y++) {
      for (uint64_t x = 0; x < img->width; x++) {
         if (nbcomp == 1) {
            // On print le pixel de coordonnée (x,y)
            uint64_t bx = x / 8; // bx-ieme bloc horizontalement
            uint64_t px = x % 8;
            uint64_t py = y % 8; // le pixel est à la coordonnée (px,py) du blob (bx,by)
            fprintf(outputfile, "%c", ycc[0][bx]->data[px][py]);
         } else if (nbcomp == 3) {
            // On print le pixel de coordonnée (x,y)
            uint64_t px, py;
            px = x / yhf;
            py = y / yvf;
            int8_t y_ycc = ycc[y_id][(py >> 3) * nb_blocYH + (px >> 3)]->data[px % 8][py % 8];
            px = x / cbhf;
            py = y / cbvf;
            int8_t cb_ycc = ycc[cb_id][(py >> 3) * nb_blocCbH + (px >> 3)]->data[px % 8][py % 8];
            px = x / crhf;
            py = y / crvf;
            int8_t cr_ycc = ycc[cr_id][(py >> 3) * nb_blocCrH + (px >> 3)]->data[px % 8][py % 8];
            rgb_t pixel_rgb;
            ycc2rgb_pixel(y_ycc, cb_ycc, cr_ycc, &pixel_rgb);
            rgb[x * 3 + 0] = pixel_rgb.r;
            rgb[x * 3 + 1] = pixel_rgb.g;
            rgb[x * 3 + 2] = pixel_rgb.b;
         }
      }
      if (nbcomp == 3) fwrite(rgb, sizeof(char), img->width * 3, outputfile);
   }
}
