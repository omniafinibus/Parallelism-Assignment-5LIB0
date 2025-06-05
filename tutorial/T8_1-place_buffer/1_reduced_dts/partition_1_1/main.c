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
    int32_t error;
    int32_t aTest[BUFFER_SIZE];
    for(size_t i=0; i<BUFFER_SIZE; i++){ aTest[i] = i; }
    token_t localToken;

    while (1)  {
        // gnow = read_global_timer();
        // xil_printf("%04u/%010u: &owner=0x%08X owner=%d waiting for flag\n", (uint32_t) (gnow >> 32), (uint32_t) gnow, &MEM->owner, MEM->owner);
        asm("wfi");
        
        start = read_global_timer();
        while (MEM->owner != CONSUMER_IS_OWNER) ; // spin lock, do nothing

        gnow = read_global_timer();
        xil_printf("%04u/%010u: unlocked!\n", (uint32_t) (gnow >> 32), (uint32_t) gnow);

        //Read the data into the provided token
        for(size_t i=0; i < BUFFER_SIZE; i++){
            localToken.data[i] = MEM->token.data[i];
        }
        localToken.timestamp = MEM->token.timestamp;

        // Reset the read flag
        MEM->read = 0;

        // Check validity
        error = 0;
        for (size_t i = 0; i < BUFFER_SIZE; i++) {
            error += (localToken.data[i] != aTest[i]);
        }
        
        if(error == 0){  
            // Update test array
            for(size_t i=0; i<BUFFER_SIZE; i++){ aTest[i] += 10; }

            gnow = read_global_timer();
            xil_printf("%04u/%010u: read token dts=%010u\n", (uint32_t) (gnow >> 32), (uint32_t) gnow, (uint32_t)(gnow-localToken.timestamp));
        }
        else{
            // Throw error
            gnow = read_global_timer();
            xil_printf("%04u/%010u: WARNING: Data not as expected. Read: [%i,%i,%i,%i,%i], expected: [%i,%i,%i,%i,%i]\n", (uint32_t) (gnow >> 32), (uint32_t) gnow, localToken.data[0], localToken.data[1], localToken.data[2], localToken.data[3], localToken.data[4], aTest[0], aTest[1], aTest[2], aTest[3], aTest[4]);
        }
        
        // give ownership back to the producer
        MEM->owner = PRODUCER_IS_OWNER;

    }

    return 0;
}
