#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <xil_printf.h>
#include "../include/image.h"

void sobel(uint8_t const volatile * const frame_in, uint32_t const xsize_in, uint32_t const ysize_in, uint32_t const bitsperpixel_in,
           uint8_t const threshold, uint8_t volatile * const frame_out)
{
  uint32_t const bytes = bitsperpixel_in/8;
  const int fxsize = 3, fysize = 3;
  int32_t const sobelx[] = {
    -1, 0, 1,
    -2, 0, 2,
    -1, 0, 1,
  };
  int32_t const sobely[] = {
     1,  2,  1,
     0,  0,  0,
    -1, -2, -1
  };

  for (uint32_t y=0; y < ysize_in; y++) {
    for (uint32_t x=0; x < xsize_in; x++) {
      for (uint32_t b=0; b < bytes; b++) {
        // don't create edge at borders
        if (x < (fxsize+1)/2 || x >= xsize_in-(fxsize+1)/2 || y < (fysize+1)/2 || y >= ysize_in-(fysize+1)/2) {
          frame_out[y*xsize_in+x] = 0;
        } else {
          int32_t xr = 0, yr = 0;
          for (int32_t ty=0; ty < fysize; ty++) {
            for (int32_t tx=0; tx < fxsize; tx++) {
              xr += sobelx[ty * fxsize + tx] * frame_in[(y + ty - fysize/2) * xsize_in + x + tx - fxsize/2];
              yr += sobely[ty * fxsize + tx] * frame_in[(y + ty - fysize/2) * xsize_in + x + tx - fxsize/2];
            }
          }
          // gradient magnitude
          double const r = sqrt(yr*yr + xr*xr); 
          // clip/saturate from int32_t to uint8_t
          if (r < threshold) frame_out[y * xsize_in + x] = 0;
          else if (r > UINT8_MAX) frame_out[y * xsize_in + x] = UINT8_MAX;
          else frame_out[y * xsize_in + x] = UINT8_MAX;
        }
      }
    }
  }
}

// uses malloc, which is probably not what you want in the embedded implementation
void overlay_sobel(uint8_t const volatile * const frame_in, uint32_t const xsize_in, uint32_t const ysize_in, uint32_t const bitsperpixel_in,
                   uint8_t const threshold, uint8_t volatile * const frame_out)
{
  uint32_t const bytes = xsize_in * ysize_in;
  uint32_t const bitsperpixel = 8;
  uint8_t * frame = (uint8_t *) frame_in;
  if (bitsperpixel_in == 24) {
    frame = (uint8_t *) malloc (bytes);
    if (frame == NULL) {
      xil_printf("overlay_sobel: cannot malloc frame\n");
      return;
    }
    greyscale(frame_in, xsize_in, ysize_in, bitsperpixel_in, frame);
  }
  uint8_t * const frame_sobel = (uint8_t *) malloc (bytes);
  if (frame_sobel == NULL) {
    xil_printf("overlay_sobel: cannot malloc frame_sobel\n");
    if (bitsperpixel_in == 24) free(frame);
    return;
  }
  sobel(frame, xsize_in, ysize_in, bitsperpixel, threshold, frame_sobel);
  overlay(frame_in, xsize_in, ysize_in, bitsperpixel_in, frame_sobel, xsize_in, ysize_in, bitsperpixel, 0, 0, 0.7, frame_out);
  if (bitsperpixel_in == 24) free(frame);
  free(frame_sobel);
}
