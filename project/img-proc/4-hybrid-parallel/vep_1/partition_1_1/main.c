#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <timers.h>
#include <xil_printf.h>
#include <platform.h>
#include <vep_shared_memory_regions.h>
#include <image.h>

#define MEM 

int main (void)
{
  uint64_t et_start, et_end, rt_start, rt_end, t;
  do {
    t = read_global_timer();
    xil_printf("%04u/%010u: waiting for image\n", (uint32_t)(t>>32), (uint32_t)t);
  } while (1);
}