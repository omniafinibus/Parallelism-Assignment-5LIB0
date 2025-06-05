#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

void scale(uint8_t const volatile * const frame_in, uint32_t const xsize_in, uint32_t const ysize_in, uint32_t const bitsperpixel_in,
           uint32_t const volatile xsize_out, uint32_t const ysize_out, uint8_t volatile * const frame_out)
{
  uint32_t const bytes = bitsperpixel_in/8;
  if (xsize_out <= xsize_in && ysize_out <= ysize_in) {
    // scale down
    for (uint32_t y = 0; y < ysize_in; y++) {
      for (uint32_t x = 0; x < xsize_in; x++) {
        for (uint32_t b = 0; b < bytes; b++) {
          frame_out[((y * ysize_out / ysize_in * xsize_out + x * xsize_out / xsize_in)) * bytes + b] = frame_in[(y * xsize_in + x) * bytes + b];
        }
      }
    }
  } else if (xsize_out > xsize_in && ysize_out > ysize_in) {
    // scale up
    for (uint32_t y = 0; y < ysize_out; y++) {
      for (uint32_t x = 0; x < xsize_out; x++) {
        for (uint32_t b = 0; b < bytes; b++) {
          frame_out[(y * xsize_out + x) * bytes + b] =
          frame_in[((y * ysize_in / ysize_out * xsize_in + x * xsize_in / xsize_out)) * bytes + b];
        }
      }
    }
  }
}
