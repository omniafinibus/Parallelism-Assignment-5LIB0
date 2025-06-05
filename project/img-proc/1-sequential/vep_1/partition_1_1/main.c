#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <timers.h>
#include <xil_printf.h>
#include <platform.h>
#include <vep_shared_memory_regions.h>
#include <image.h>

#define MEM vep_memshared0_shared_region
#define MEM11 vep_tile1_partition1_shared_region

 
int main (void)
{
  uint64_t et_start, et_end, rt_start, rt_end, t;
  uint8_t 
  do {
    t = read_global_timer();
    xil_printf("%04u/%010u: waiting for image\n", (uint32_t)(t>>32), (uint32_t)t);

    // wait for image
    while (!MEM->newImage){;}

    // convert to greyscale
    uint32_t const orig_bits = MEM->bitsperpixel;
    if (MEM->bitsperpixel != 8) {
      greyscale(MEM->image, MEM->xsize, MEM->ysize, MEM->bitsperpixel, MEM->image);
      MEM->bitsperpixel = 8;
    }
    
    // convolution (average or gaussian blur)
    uint32_t size;
    uint8_t * convFrame;
    double const * filter;
    convFrame = (uint8_t *) malloc (MEM->xsize*MEM->ysize*(MEM->bitsperpixel/8));
    if (orig_bits == 8) {
      filter = conv_avgxy1;
      size = 1;
    } else {
      filter = conv_gaussianblur5;
      size = 5;
    }
    convolution(MEM->image, MEM->xsize, MEM->ysize, MEM->bitsperpixel, filter, size, size, convFrame);

    // sobel
    uint8_t * const sobelFrame = (uint8_t *) malloc (MEM->xsize*MEM->ysize*(MEM->bitsperpixel/8));
    uint8_t const threshold = (orig_bits == 8 ? 100 : 128);
    sobel(MEM->image, MEM->xsize, MEM->ysize, MEM->bitsperpixel, threshold, sobelFrame);

    // overlay
    overlay(convFrame, MEM->xsize, MEM->ysize, MEM->bitsperpixel, sobelFrame, MEM->xsize, MEM->ysize, MEM->bitsperpixel, 0, 0, 0.7, MEM->image);
    MEM->newImage = 0;
    MEM11->done = 1;
  } while (1);
}