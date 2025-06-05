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
  int32_t a, b, c;
  a = 3;
  b = 4;
  c = a + b;

  MEM->d = c;

  // print the address and the value of d, and
  xil_printf("&d=0x%08p d=%i", &(MEM->d), MEM->d);

  // then print out the values of a, b, c
  xil_printf(" &a=0x%08p a=%i &b=0x%08p b=%i &c=0x%08p c=%i\n", &a, a, &b, b, &c, c);

  // sleep  (asm("wfi");)
  asm("wfi");

  // then print the values again
  // print the address and the value of d, and
  xil_printf("&d=0x%08p d=%i", &(MEM->d), MEM->d);

  // then print out the values of a, b, c
  xil_printf(" &a=0x%08p a=%i &b=0x%08p b=%i &c=0x%08p c=%i\n", &a, a, &b, b, &c, c);

  return 0;
}
