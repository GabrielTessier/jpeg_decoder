#pragma once

#include <stdio.h>
#include <entete.h>
#include <bitstream.h>

erreur_t decode_progressive_image(FILE *infile, img_t *img);
