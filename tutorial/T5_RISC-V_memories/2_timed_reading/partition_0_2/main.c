#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <timers.h>
#include <xil_printf.h>
#include <platform.h>
#include <vep_shared_memory_regions.h>

#define ITERATIONS 6
volatile uint32_t *e = VEP_TILE0_PARTITION2_SHARED_REGION_LOCAL_START;

int main ( void )
{
  xil_printf("running on tile %d partition %d\n", TILE_ID, PARTITION_ID);
  volatile int32_t a, b, c;
  a = 3;
  b = 4;
  c = a + b;
  MEM->d = c;

  // print out the time to read the variabeles 6 times in a loop
  // local variable a
  uint64_t start, stop;
  for(size_t i = 1; i <= ITERATIONS; i++){
    b = 0;
    start = read_global_timer();
    b = a;
    stop = read_global_timer();
    xil_printf("read a %uth time, took %u global cycles\n", i, stop - start);
  }  

  // variable d, accessed via the ring
  for(size_t i = 1; i <= ITERATIONS; i++){
    b = 0;
    start = read_global_timer();
    b = MEM->d;
    stop = read_global_timer();
    xil_printf("read d %uth time, took %u global cycles\n", i, stop - start);
  }  

  // variable e, accessed locally via the LMB
  for(size_t i = 1; i <= ITERATIONS; i++){
    b = 0;
    start = read_global_timer();
    b = *e;
    stop = read_global_timer();
    xil_printf("read d %uth time (LMB), took %u global cycles\n", i, stop - start);
  }  

  // variable e, accessed via the ring
  for(size_t i = 1; i <= ITERATIONS; i++){
    b = 0;
    start = read_global_timer();
    b = vep_tile0_partition2_shared_region->e;
    stop = read_global_timer();
    xil_printf("read e %uth time (ring), took %u global cycles\n", i, stop - start);
  }  

  // xil_printf("&d=0x%08p d=%i &a=0x%08p a=%i &b=0x%08p b=%i &c=0x%08p c=%i\n", &(MEM->d), MEM->d, &a, a, &b, b, &c, c);
  asm("wfi");
  // xil_printf("&d=0x%08p d=%i &a=0x%08p a=%i &b=0x%08p b=%i &c=0x%08p c=%i\n", &(MEM->d), MEM->d, &a, a, &b, b, &c, c);
}
