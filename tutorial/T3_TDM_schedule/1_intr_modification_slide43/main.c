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
    // Set the ARM processor in sleep mode until an interrupt occurs (Wait For Interrupt)
    asm("wfi"); 
    uint64_t gnow = read_global_timer();
    xil_printf("[diff]: [%010u]\n", (uint32_t)(gnow-gprev)); 

    //Difference should always be 43000 (alligned with the TDM schedule)
    gprev = gnow;
  }

  return 0;
}
