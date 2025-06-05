#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <timers.h>
#include <xil_printf.h>
#include <platform.h>
#include <vep_shared_memory_regions.h>

#define ITERATIONS 100

uint64_t volatile const * const g_timer = (uint64_t *) GLOBAL_TIMER;
uint64_t volatile const * const p_timer = (uint64_t *) PARTITION_TIMER;

int main ( void )
{
  xil_printf("running on tile %d partition %d\n", TILE_ID, PARTITION_ID);

  //Initialize gnow array as 0
  uint64_t a_gnow[ITERATIONS];
  memset(a_gnow, 0, ITERATIONS);

  uint64_t * lastAddress = &a_gnow[ITERATIONS-1];
  uint64_t * gnow = &a_gnow[0];

  // Run main code 
  while(gnow <= lastAddress) {
    asm("wfi"); // once per slot
    *(gnow++) = read_global_timer(); //Save execution time
  }
  
  // Print out all differences between iterations and global clock values
  gnow = &a_gnow[1];
  while(gnow <= lastAddress) {
    // xil_printf("now& | prev& | first& | last&: %010u | %010u | %010u | %010u\n", gnow++, gnow-2, &a_gnow[0], &a_gnow[ITERATIONS-1]);
    xil_printf("[top32b-bot32b|diff]: [%010u-%010u|%010u]\n", (uint32_t)(*gnow>>32), (uint32_t)*gnow, (uint32_t)(*(gnow++)-*(gnow-2)));
  }

  return 0;
}
