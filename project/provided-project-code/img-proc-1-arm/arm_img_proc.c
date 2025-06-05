#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include "image_processing_functions.h"

int main(int argc, char ** argv)
{
  if (argc < 2) {
    fprintf(stderr, "Usage: %s infile ...\n" , argv[0]);
    return 1;
  }

  int nr_files;
  nr_files = argc -1;

  for (int f=1; f <= nr_files; f++) {
    uint32_t xsize1, ysize1, bitsperpixel1;
    uint32_t xsize2, ysize2, bitsperpixel2;
    uint32_t xsize3, ysize3, bitsperpixel3;
    uint32_t xsize4, ysize4, bitsperpixel4;
    uint32_t xsize5, ysize5, bitsperpixel5;

    // read BMP
    printf("Processing file %s\n",argv[f]);
    uint8_t * frame1 = NULL;
    if (!readBMP(argv[f], &frame1, &xsize1, &ysize1, &bitsperpixel1)) {
      printf("Cannot read file %s\n",argv[f]);
      return 1;
    }

    uint32_t bytes2, bytes3;
    uint8_t * frame2, * frame3;

    // convert to greyscale
    uint32_t const orig_bits = bitsperpixel1;
    xsize2 = xsize1;
    ysize2 = ysize1;
    bitsperpixel2 = 8;
    bytes2 = xsize2*ysize2*(bitsperpixel2/8);
    frame2 = (uint8_t *) malloc (bytes2);
    greyscale(frame1, xsize1, ysize1, bitsperpixel1, frame2);

    // convolution (average or gaussian blur)
    xsize3 = xsize2;
    ysize3 = ysize2;
    bitsperpixel3 = bitsperpixel2;
    bytes3 = xsize3*ysize3*(bitsperpixel3/8);
    frame3 = (uint8_t *) malloc (bytes3);
    double const * filter;
    uint32_t size;
    if (orig_bits == 8) {
      filter = conv_avgxy3;
      size = 3;
    } else {
      filter = conv_gaussianblur5;
      size = 5;
    }
    convolution(frame2, xsize2, ysize2, bitsperpixel2, filter, size, size, frame3);

    // sobel
    xsize4 = xsize3;
    ysize4 = ysize3;
    bitsperpixel4 = bitsperpixel3;
    uint32_t const bytes4 = xsize4*ysize4*(bitsperpixel4/8);
    uint8_t * const frame4 = (uint8_t *) malloc (bytes4);
    uint8_t const threshold = (orig_bits == 8 ? 100 : 128);
    sobel(frame3, xsize3, ysize3, bitsperpixel3, threshold, frame4);

    // overlay
    xsize5 = xsize4;
    ysize5 = ysize4;
    bitsperpixel5 = bitsperpixel4;
    uint32_t const bytes5 = xsize4*ysize4*(bitsperpixel4/8);
    uint8_t * const frame5 = (uint8_t *) malloc (bytes5);
    overlay(frame3, xsize3, ysize3, bitsperpixel3, frame4, xsize4, ysize4, bitsperpixel4, 0, 0, 0.7, frame5);

    // write BMP
    // note that reading & then writing doesn't always result in the same image
    // - a grey-scale (8-bit pixel) image will be written as a 24-bit pixel image too
    // - the header of colour images may change too
    char outfile[200];
    const char *prefix = "out-";
    strcpy(outfile, prefix);
    strcpy(&outfile[strlen(prefix)],argv[f]);
    outfile[strlen(argv[f])+strlen(prefix)] = '\0';
    (void) writeBMP(outfile, frame5, xsize5, ysize5, bitsperpixel5);
  }
  return 0;
}
