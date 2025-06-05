#ifndef __ARM_SHARED_MEMORY_SYSTEMH_
#define __ARM_SHARED_MEMORY_SYSTEMH_

#include <stdint.h>

/*
 * System interface to shared memories - unrestricted read/write access to all shared memories
 */

typedef enum CommandType {
	CT_NONE      = 0,
	CT_CLEAR     = 1,
	CT_PATTERN   = 2,
	CT_INCREMENT = 3,
	CT_FILE      = 4,
	CT_READ	     = 5
} CommandType;

struct arm_shared_t {
	int file_descriptor;
	uint32_t address;
	uint32_t length;
	void *mmaped_region;
};
/**
 * Object handle.
 */
typedef struct arm_shared_t arm_shared;

/**
 * @param handle a handle to store it internal state.
 * @param address address to access (should be in the shared memory range).
 * @param length the length of the section to access.
 *
 * Open a shared memory for reading and writing.
 *
 * @returns a pointer to the shared memory region.
 */

void *arm_shared_init ( arm_shared *handle, const uint32_t address, const uint32_t length );


/**
 * @param handle a handle to its internal state.
 *
 * closes the shared memory region, invalidating the previously accessed pointer.
 */
void arm_shared_close ( arm_shared *handle);


/**
 * High-level interface to clear/pattern/increment/read.
 * Use the array to pass out read data & to pass in file data.
 */
uint32_t shared_mem_rw(int verbose, CommandType command_type, uint32_t start, uint32_t length, uint32_t pattern, uint8_t *array);

#endif // ARM_READ_SHARED_H
