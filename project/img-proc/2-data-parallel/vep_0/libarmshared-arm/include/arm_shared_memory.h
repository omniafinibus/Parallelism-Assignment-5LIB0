#ifndef __ARM_SHARED_MEMORYH_
#define __ARM_SHARED_MEMORYH_

#include <stdint.h>
#include "arm_shared_memory_system.h"
#include "vep_shared_memory_regions.h"

// use arm_shared_memory_init to map a region in the shared memory memshared0 to a memory region in the memory of the ARM
// Linux ensures that the original data in memshared0 and the copy in the ARM caches remain consistent
// call arm_shared_memory_close before exiting your program
// example:
// vep_memshared0_t * data_in_shared_mem = arm_shared_memory_init();
// printf("the value of flag in the shared memory vep_memshared0 is %d\n",data_in_shared_mem->flag);
// arm_shared_memory_close();
extern vep_memshared0_shared_region_t * arm_shared_memory_init();
extern void arm_shared_memory_close();

// these functions operate on words, i.e. 4 bytes
// - start address must be word aligned (multiple of 4)
// - length must be a multiple of 4
// - return 0 on success, non-zero otherwise
// fill start..start+length-1 with write_data
extern uint32_t arm_shared_memory_write(uint32_t start, uint32_t length, uint32_t write_data);
// copy the array write_data into start..start+length-1
extern uint32_t arm_shared_memory_write_array(uint32_t start, uint32_t length, uint8_t *write_data);
// copy into start..start+length-1 into the array write_data 
// return_array must point to length uint8_t's
extern uint32_t arm_shared_memory_read(uint32_t start, uint32_t length, uint8_t *read_data);

#endif
