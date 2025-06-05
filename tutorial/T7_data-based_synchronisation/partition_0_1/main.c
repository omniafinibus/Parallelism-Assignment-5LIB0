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
  
  uint64_t gnow, start;
  gnow = read_global_timer();
  xil_printf("%04u/%010u: &sync=0x%08X sync=%d waiting for flag\n", (uint32_t) (gnow >> 32), (uint32_t) gnow, &MEM->sync, MEM->sync);
  
  start = read_global_timer();
  while (vep_memshared0_shared_region->sync != 2) ; // spin lock, do nothing
  
  gnow = read_global_timer();
  xil_printf("%04u/%010u: &sync=0x%08X sync=%d unlocked!\n", (uint32_t) (gnow >> 32), (uint32_t) gnow, &MEM->sync, MEM->sync);

  return 0;
}
