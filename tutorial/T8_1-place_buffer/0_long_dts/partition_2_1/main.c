#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <timers.h>
#include <xil_printf.h>
#include <platform.h>
#include <vep_shared_memory_regions.h>
#include "shared_datatypes.h"

int main ( void )
{
    xil_printf("running on tile %d partition %d\n", TILE_ID, PARTITION_ID);

    uint64_t gnow, start;

    gnow = read_global_timer();
    xil_printf("%04u/%010u: &owner=0x%08X owner=%d waiting for flag\n", (uint32_t) (gnow >> 32), (uint32_t) gnow, &MEM->owner, MEM->owner);

    start = read_global_timer();
    while (MEM->owner != PRODUCER_IS_OWNER) ; // spin lock, do nothing

    gnow = read_global_timer();
    xil_printf("%04u/%010u: &owner=0x%08X owner=%d unlocked!\n", (uint32_t) (gnow >> 32), (uint32_t) gnow, &MEM->owner, MEM->owner);

    // Change the data
    for(size_t i=0; i < BUFFER_SIZE; i++){
        MEM->token.data[i] = i;
    }
    MEM->token.timestamp = read_global_timer();
    MEM->read = 1;
    MEM->owner = CONSUMER_IS_OWNER;

    // Print written data
    xil_printf("%04u/%010u: wrote token ts=%010i data=%i,%i,%i,%i,%i\n", (uint32_t) (gnow >> 32), (uint32_t) gnow, MEM->token.timestamp, MEM->token.data[0], MEM->token.data[1], MEM->token.data[2], MEM->token.data[3], MEM->token.data[4]);

    while (1)  {
        gnow = read_global_timer();
        xil_printf("%04u/%010u: &owner=0x%08X owner=%d waiting for flag\n", (uint32_t) (gnow >> 32), (uint32_t) gnow, &MEM->owner, MEM->owner);

        start = read_global_timer();
        while (MEM->owner != PRODUCER_IS_OWNER) ; // spin lock, do nothing

        gnow = read_global_timer();
        xil_printf("%04u/%010u: &owner=0x%08X owner=%d unlocked!\n", (uint32_t) (gnow >> 32), (uint32_t) gnow, &MEM->owner, MEM->owner);

        if(MEM->read == 1) {
            gnow = read_global_timer();
            xil_printf("%04u/%010u: WARNING! 1-place buffer is being overwritten without it being read.\n", (uint32_t) (gnow >> 32), (uint32_t) gnow);
        }

        // Change the data
        for(size_t i=0; i < BUFFER_SIZE; i++){
            MEM->token.data[i] += 10;
        }

        // Attach the update time 
        MEM->token.timestamp = read_global_timer();
        MEM->read = 1;
        MEM->owner = CONSUMER_IS_OWNER;

        // Print written data
        xil_printf("%04u/%010u: wrote token ts=%010i data=%i,%i,%i,%i,%i\n", (uint32_t) (gnow >> 32), (uint32_t) gnow, (uint32_t)MEM->token.timestamp, MEM->token.data[0], MEM->token.data[1], MEM->token.data[2], MEM->token.data[3], MEM->token.data[4]);
    }
    return 0;
}
