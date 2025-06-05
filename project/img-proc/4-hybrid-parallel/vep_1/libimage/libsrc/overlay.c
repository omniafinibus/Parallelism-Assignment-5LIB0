#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <xil_printf.h>

void overlay(uint8_t const volatile * const frame_in1, uint32_t volatile xsize_in1, uint32_t const ysize_in1, uint32_t const bitsperpixel_in1,
             uint8_t const volatile * const frame_in2, uint32_t volatile xsize_in2, uint32_t const ysize_in2, uint32_t const bitsperpixel_in2,
             uint32_t const xoffset, uint32_t yoffset, double const ratio, uint8_t volatile * const frame_out)
{
  if (xoffset + xsize_in1 < xsize_in2 || yoffset + ysize_in1 < ysize_in2 ||
      bitsperpixel_in1 < bitsperpixel_in2 || ratio < 0 || ratio > 1) {
    xil_printf("overlay: invalid parameters\n");
    return;
  }
  uint32_t const bytes = bitsperpixel_in1/8;
  // frame_in & frame_out can be the same
  // it's therefore crucial to have x & y incrementing
  for (uint32_t y = 0; y < ysize_in1; y++) {
    for (uint32_t x = 0; x < xsize_in1; x++) {
      for (uint32_t b = 0; b < bytes; b++) {
        if (x >= xoffset && x < xoffset + xsize_in2 && y >= yoffset && y < yoffset + ysize_in2) {
          if (bitsperpixel_in1 == bitsperpixel_in2) {
            frame_out[(y * xsize_in1 + x) * bytes + b] =
              ratio * frame_in1[(y * xsize_in1 + x) * bytes + b] +
              (1 - ratio) * frame_in2[((y - yoffset) * xsize_in2 + x - xoffset) * bytes + b];
          } else {
            // frame 1 is RGB, frame 2 is grey
            frame_out[(y * xsize_in1 + x) * bytes + b] =
              ratio * frame_in1[(y * xsize_in1 + x) * bytes + b] +
              (1 - ratio) * frame_in2[(y - yoffset) * xsize_in2 + x - xoffset];
          }
        } else {
          frame_out[(y * xsize_in1 + x) * bytes + b] = frame_in1[(y * xsize_in1 + x) * bytes + b];
        }
      }
    }
  }
}