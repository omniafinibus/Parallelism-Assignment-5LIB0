#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <timers.h>
#include <xil_printf.h>
#include <platform.h>
#include <vep_shared_memory_regions.h>

int main ( void )
{
  xil_printf("running on tile %d partition %d\n", TILE_ID, PARTITION_ID);
  extern vep_memshared0_shared_region_t volatile * const vep_memshared0_shared_region;
  int32_t a, b, c;

  a = 3;
  b = 4;
  c = a + b;

  xil_printf("&d=0x%08p d=%i &a=0x%08p a=%i &b=0x%08p b=%i &c=0x%08p c=%i\n", &(vep_memshared0_shared_region->d), vep_memshared0_shared_region->d, &a, a, &b, b, &c, c);
  return 0;
}
