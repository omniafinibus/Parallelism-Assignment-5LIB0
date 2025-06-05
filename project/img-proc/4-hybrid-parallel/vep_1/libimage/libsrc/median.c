#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#define MIN(x,y) ((x)<(y)?(x):(y))
#define MAX(x,y) ((x)>(y)?(x):(y))
#define min(i,j) a[i] = MIN(a[i],a[j]);
#define max(i,j) a[j] = MAX(a[i],a[j]);
#define swap(i,j) tmp = MIN(a[i],a[j]); a[j] = MAX(a[i],a[j]); a[i] = tmp;

int medianSelection9(uint8_t a[]) {
  uint8_t tmp;
  swap(0,1); swap(3,4); swap(6,7);
  swap(1,2); swap(4,5); swap(7,8);
  swap(0,1); swap(3,4); swap(6,7);
  max(0,3);  max(3,6);
  swap(1,4); min(4,7);  max(1,4);
  min(5,8);  min(2,5);
  swap(2,4); min(4,6);  max(2,4);
  return a[4];
}

void median_filter(uint8_t const volatile * const frame_in, uint32_t const xsize_in, uint32_t const ysize_in, uint32_t const bitsperpixel_in, uint8_t volatile * const frame_out)
{
  uint32_t const bytes = bitsperpixel_in/8;

  // in bitmaps the bottom line of the image is at the beginning of the file
  // but for this filter we can process in any order
  for (uint32_t y = 0; y < ysize_in; y++) {
    for (uint32_t x = 0; x < xsize_in; x++) {
      for (uint32_t b = 0; b < bytes; b++) {
        uint8_t nr[9] = { 0 };
        uint32_t i = 0;
        for (int32_t ty = -1; ty <= 1; ty++) {
          for (int32_t tx = -1; tx <= 1; tx++) {
            if (((int32_t) x)+tx >= 0 && x+tx < xsize_in && ((int32_t) y)+ty >= 0 && y+ty < ysize_in) {
              nr[i] = frame_in[((y+ty) * xsize_in + (x+tx)) * bytes + b];
              i++;
            }
          }
        }
        // skip border
        if (y == 0 || y == ysize_in-1 || x == 0 || x == xsize_in -1)
          frame_out[(y * xsize_in + x) * bytes + b] = frame_in[(y * xsize_in + x) * bytes + b];
        else
          frame_out[(y * xsize_in + x) * bytes + b] = medianSelection9(nr);
      }
    }
  }
}
