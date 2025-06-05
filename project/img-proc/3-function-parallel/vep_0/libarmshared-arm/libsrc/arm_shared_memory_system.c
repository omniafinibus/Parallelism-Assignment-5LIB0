#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "arm_shared_memory_system.h"

#include "platform.h"
#include "cheapout.h"

#define TILE_SHARED_START_LOCAL_ARM 0x40000000

void * arm_shared_init ( arm_shared *handle, const uint32_t address,  const uint32_t length )
{
	if ( handle == NULL ) {
		fprintf(stderr, "You need to pass a valid handle to %s\n", __FUNCTION__);
		exit(EXIT_FAILURE);
	}

	uint32_t allowed_start = TILE_SHARED_START+sizeof(shared_memory_map);
	uint32_t allowed_end = TILE_SHARED_START+SHARED_MEMORY_SIZE;

	if ( !(address >= allowed_start && address < allowed_end )) {

		fprintf(stderr, "The requested memory address is out of range of the shared memory.\n");
		fprintf(stderr, "allowed-end=%08X > start-address=%08X >= allowed-start=%08X.\n", allowed_end, address, allowed_start);
		exit ( EXIT_FAILURE );
	}
	if ( !( (address+length-1) >= allowed_start && (address+length-1) < allowed_end )) {
		fprintf(stderr, "The requested memory range is out of range of the shared memory.\n");
		fprintf(stderr, "allowed-end=%08X > end-address=%08X >= allowed-start=%08X.\n", allowed_end, address+length-1, allowed_start);
		exit ( EXIT_FAILURE );
	}

	handle->address = address; 
	handle->length = length;
	handle->file_descriptor = open("/dev/mem", O_RDWR|O_SYNC);
	if(handle->file_descriptor < 0) {
		fprintf(stderr, "FAILED open memory: %s, please run with sufficient permissions (sudo).\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	long page_size = sysconf(_SC_PAGE_SIZE);
	//fprintf(stderr, "Page size: %ld\r\n", page_size);

	uint32_t start_address = TILE_SHARED_START_LOCAL_ARM + (handle->address-TILE_SHARED_START);
	uint32_t page_offset = start_address%page_size; 
	//fprintf(stderr, "Page offset: %u\r\n", page_offset);
	start_address -= page_offset;
	handle->length += page_offset;
	//fprintf(stderr, "Start address: %08X\r\n", start_address);

	handle->mmaped_region = mmap(NULL, 
			handle->length,
			PROT_READ|PROT_WRITE, MAP_SHARED,
			handle->file_descriptor,
			start_address);

	if ( handle->mmaped_region == MAP_FAILED) {
		fprintf(stderr, "FAILED to memory map requested region: %s\n", strerror(errno));
		close(handle->file_descriptor);
		exit(EXIT_FAILURE);
	}
	return (void*)(((uint32_t)(handle->mmaped_region))+page_offset);
}

void arm_shared_close ( arm_shared *handle ) 
{
	if ( handle == NULL ) {
		fprintf(stderr, "You need to pass a valid handle to %s\n", __FUNCTION__);
		exit(EXIT_FAILURE);
	}
	if ( handle->mmaped_region != MAP_FAILED) {
		munmap(handle->mmaped_region, handle->length);
	}
	if ( handle->file_descriptor >= 0 ) {
		close ( handle->file_descriptor );
	}
}

// system function, not for users

uint32_t shared_mem_rw(int verbose, CommandType command_type, uint32_t start, uint32_t length, uint32_t pattern, uint8_t *array)
{
	start = start;
	length = length;
	pattern = pattern;
	if (command_type == CT_CLEAR) {
		pattern = 0;
		command_type = CT_PATTERN;
	}

	if ( start%4 != 0 ) {
		fprintf(stderr, "The start address needs to be word-aligned.\n");
		return (EXIT_FAILURE);
	}
	if ( command_type < CT_FILE && length%4 != 0 ) {
		fprintf(stderr, "The length needs to be multiple of 4 (one word).\n");
		return (EXIT_FAILURE);
	}
	uint32_t const memory = NUM_TILES;
	// for now don't allow accessing tile memories, and there's only one shared memory
	if (memory < NUM_TILES || memory >= NUM_SHARED_MEMORIES) {
		fprintf(stderr, "error: invalid memory=%u\n", memory);
		return EXIT_FAILURE;
	}
	if (start < shared_mems_start[memory] || start+length >= shared_mems_start[memory] + shared_mems_size[memory]) {
		fprintf(stderr, "error: invalid address range (start=0x%08X length=0x%08X) for memory %u (start=0x%08X size=0x%0X)\n",
				start, length, memory, shared_mems_start[memory], shared_mems_size[memory]);
		return EXIT_FAILURE;
	}

	struct arm_shared_t handle = {0, }; 
	volatile uint32_t *ocm = (volatile uint32_t*)arm_shared_init ( &handle, start, length );
	volatile uint8_t *ocmb = (volatile uint8_t*)ocm;

	uint32_t index = 0;
	for ( ; index < (length/4); index++ ) {
		if ( verbose && command_type != CT_READ) {
			printf("\r0x%08X", index*4);
			fflush(NULL);
		}
		switch ( command_type ) {
			case CT_READ:
				array[index*4+0] = ocmb[index*4+0];
				array[index*4+1] = ocmb[index*4+1];
				array[index*4+2] = ocmb[index*4+2];
				array[index*4+3] = ocmb[index*4+3];
				break;
			case CT_INCREMENT: 
				ocm[index] = index;
				break;
			case CT_PATTERN: 
				ocm[index] = pattern;
				break;
			case CT_FILE:
				ocmb[index*4+0] = array[index*4+0];
				ocmb[index*4+1] = array[index*4+1];
				ocmb[index*4+2] = array[index*4+2];
				ocmb[index*4+3] = array[index*4+3];
				break;
			default:
			break;
		}	
	}
	index*=4;
	if ( command_type == CT_READ ) {
		while ( index < length) {
			array[index] = ocmb[index];
			index++;
		}
	}
	if ( command_type == CT_FILE ) {
		while ( index < length) {
			ocmb[index] = array[index];
			index++;
		}
	}
	if ( verbose && command_type != CT_READ) printf("\r0x%08X / %d bytes\n", index, index);
	arm_shared_close ( &handle );
	return (EXIT_SUCCESS);
}