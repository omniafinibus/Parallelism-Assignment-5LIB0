#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <timers.h>
#include <xil_printf.h>
#include <platform.h>
#include <vep_shared_memory_regions.h>

uint64_t volatile const * const g_timer = (uint64_t *) GLOBAL_TIMER;
uint64_t volatile const * const p_timer = (uint64_t *) PARTITION_TIMER;

int main ( void )
{
  xil_printf("running on tile %d partition %d\n", TILE_ID, PARTITION_ID);

  uint64_t gnow = *g_timer;
  uint64_t gprev = 0; // Overflows every (2^32-1)/40M seconds = 107.37 seconds
 
  xil_printf("timer: %024Z\n", gnow);
  xil_printf("timer: %024Y\n", (int64_t)gnow);

  while (1) {
      uint64_t gnow = read_global_timer();
      xil_printf("global timer top 32 bits: %010u\n", (uint32_t) (gnow>>32)); // print out the global timer and pad with 0s if number is less then 10 characters long (unsigned)
      xil_printf("global timer bottom 32 bits: %010u\n", (uint32_t)gnow);
      xil_printf("global timer diff: %010u\n", (uint32_t)(gnow-gprev));
      gprev = gnow;
  }

  return 0;
}
