#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "arm_shared_memory.h"

arm_shared shared_memory_handle = { 0, }; 

vep_memshared0_shared_region_t * arm_shared_memory_init()
{
  if (shared_memory_handle.mmaped_region != NULL) arm_shared_close(&shared_memory_handle);
  return (vep_memshared0_shared_region_t *) arm_shared_init (&shared_memory_handle, VEP_MEMSHARED0_SHARED_REGION_REMOTE_START, sizeof(vep_memshared0_shared_region_t));
}

void arm_shared_memory_close()
{
  arm_shared_close(&shared_memory_handle);
}

uint32_t arm_shared_memory_write(uint32_t start, uint32_t length, uint32_t write_data)
{
	uint32_t m;
	for (m = NUM_TILES; m < NUM_SHARED_MEMORIES; m++) {
		if (start >= vep_memories_shared_remote_start[m][0] &&
		    start+length-1 < vep_memories_shared_remote_start[m] [0]+ vep_memories_shared_remote_size[m][0]) {
			break;
		}
	}
	if (m == NUM_SHARED_MEMORIES) {
		fprintf(stderr, "error: invalid address range (start=0x%08X length=0x%08X is not range of any shared memory region\n", start, length);
		return EXIT_FAILURE;
	}
	return shared_mem_rw(0 /*quiet*/, CT_PATTERN, start, length, write_data, NULL /*no return data*/);
}

uint32_t arm_shared_memory_write_array(uint32_t start, uint32_t length, uint8_t *write_data)
{
	uint32_t m;
	for (m = NUM_TILES; m < NUM_SHARED_MEMORIES; m++) {
		if (start >= vep_memories_shared_remote_start[m][0] &&
		    start+length-1 < vep_memories_shared_remote_start[m] [0]+ vep_memories_shared_remote_size[m][0]) {
			break;
		}
	}
	if (m == NUM_SHARED_MEMORIES) {
		fprintf(stderr, "error: invalid address range (start=0x%08X length=0x%08X is not range of any shared memory region\n", start, length);
		return EXIT_FAILURE;
	}
	return shared_mem_rw(0 /*quiet*/, CT_FILE, start, length, 0 /*not used*/, write_data);
}

uint32_t arm_shared_memory_read(uint32_t start, uint32_t length, uint8_t *read_data)
{
	uint32_t m;
	for (m = NUM_TILES; m < NUM_SHARED_MEMORIES; m++) {
		if (start >= vep_memories_shared_remote_start[m][0] &&
		    start+length-1 < vep_memories_shared_remote_start[m] [0]+ vep_memories_shared_remote_size[m][0]) {
			break;
		}
	}
	if (m == NUM_SHARED_MEMORIES) {
		fprintf(stderr, "error: invalid address range (start=0x%08X length=0x%08X is not range of any shared memory region\n", start, length);
		return EXIT_FAILURE;
	}
  	return shared_mem_rw(0 /*quiet*/, CT_READ, start, length, 0 /*not used*/, read_data);
}
