#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <arm_shared_memory.h>
/* warning: this is the RISC-V memory map!
 * use only in conjuction with arm_shared_memory_write/read
 */
#include "vep_memory_map.h"


// two useful functions
extern uint32_t readBMP(char const * const file, uint8_t ** outframe, uint32_t * const x_size, uint32_t * const y_size, uint32_t * const bitsperpixel);
extern uint32_t writeBMP(char const * const file, uint8_t const * const outframe, uint32_t const x_size, uint32_t const y_size, uint32_t const bitsperpixel);

int main(int argc, char ** argv)
{
  if (argc < 2) {
    fprintf(stderr, "Usage: %s infile ...\n" , argv[0]);
    return 1;
  }

  uint32_t nr_files;
  nr_files = argc -1;

  // map the region in shared memory memshared0 to a memory region in the memory of the ARM
  // cast the resulting pointer to the same data structure vep_memshared0_t that the RISC-V uses
  // Linux ensures that the original data in memshared0 and the copy in the ARM caches remain consistent
  vep_memshared0_shared_region_t * data_in_shared_mem = arm_shared_memory_init();

  for (uint32_t f=1; f <= nr_files; f++) {
    // read BMP
    uint32_t xsize_snd, ysize_snd, bitsperpixel_snd;
    uint8_t *frame_snd = NULL;

    // read file
    if (!readBMP(argv[f], &frame_snd, &xsize_snd, &ysize_snd, &bitsperpixel_snd)) {
      printf("Cannot read file %s\n",argv[f]);
      arm_shared_memory_close();
      return 1;
    }
    printf("Processing file %s\n",argv[f]);
    uint32_t const bytes_snd = xsize_snd*ysize_snd*(bitsperpixel_snd/8);
    // check if it fits in the memory on the RISC-V side
    // send image to RISC-V
    // wait for the RISC-V to produce the output data

    // write BMP
    // note that reading & then writing doesn't always result in the same image
    // - a grey-scale (8-bit pixel) image will be written as a 24-bit pixel image too
    // - the header of colour images may change too
    char outfile[200];
    const char *prefix = "out-";
    strcpy(outfile, prefix);
    strcpy(&outfile[strlen(prefix)],argv[f]);
    outfile[strlen(argv[f])+strlen(prefix)] = '\0';
    if (writeBMP(outfile, ...)) {
      printf("Cannot write file %s\n",outfile);
      arm_shared_memory_close();
      return 1;
    }
  }
  // close the shared memory handle before exiting
  arm_shared_memory_close();
  return 0;
}
