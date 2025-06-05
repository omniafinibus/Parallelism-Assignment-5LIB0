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

  uint64_t gnow;
  gnow = read_global_timer();
  xil_printf("%04u/%010u: &sync=0x%08X sync=%d\n", (uint32_t) (gnow >> 32), (uint32_t) gnow, &MEM->sync, MEM->sync);
  MEM->sync = PARTITION_ID;

  gnow = read_global_timer();
  xil_printf("%04u/%010u: &sync=0x%08X sync=%d\n", (uint32_t) (gnow >> 32), (uint32_t) gnow, &MEM->sync, MEM->sync);

  asm("wfi"); // wait until the start of the next slot
  gnow = read_global_timer();
  xil_printf("%04u/%010u: &sync=0x%08X sync=%d\n", (uint32_t) (gnow >> 32), (uint32_t) gnow, &MEM->sync, MEM->sync);

  return 0;
}

