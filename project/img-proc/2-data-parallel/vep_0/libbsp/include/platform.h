#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#include <stdint.h>

#define FREQUENCY 40000000      /* cycles/sec */
#define NUM_TILES 3             /* # RISC-V tiles */
#define NUM_TDM_CYCLES 10000000 /* max length of TDM table */
#define NUM_SLOTS 32            /* # TDM slots */
#define NUM_VPS 4               /* # running user VEPs numbered 1..4, excluding the system VEP 0 */
#define NUM_PARTITIONS NUM_VPS  /* alias */
#define NUM_VEPS 32             /* max # of VEP directories, this may be more than NUM_VPS */
#define NUM_MMU_ENTRIES 16

#define CHEAP_BUFFER_SIZE_SYS 848
#define CHEAP_BUFFER_SIZE_STDIO 256 
#define CHEAP_BUFFER_SIZE 224 
#define PLATFORM_OCM_MEMORY_LOC 0xB0000000 /* from RISC-V */

#define VKERNEL_CYCLES 2000	            /* cycles for the vkernel slot */
#define SYS_APP_MEMORY 32               /* KB required by system application 0 */
#define SYS_APP_CYCLES 5000             /* first slot of this many cycles is reserved for the system application in the TDM table */
#define DEFAULT_APP_STACK 4             /* KB */
#define DEFAULT_APP_SLOT_CYCLES 20000   /* per slot, currently there's no min (except >0) or max */
#define NUM_SHARED_MEMORIES 4           /* includes RISC-V tile memories */
#define MIN_APP_SHARED_MEMORY 1024      /* B */

/***** memory map - physical addresses *****/
#define CHEAP_VP_STDOUT_SIZE (512-sizeof(cheap_t))
#define CHEAP_VP_USER_SIZE (1024-sizeof(cheap_t))
#define TILE0_IDMEM_START 0x80000000
#define TILE1_IDMEM_START 0x90000000
#define TILE2_IDMEM_START 0xA0000000
// TODO: here assume one shared memory -- should use shared_mems*[]
#define TILE_SHARED_START PLATFORM_OCM_MEMORY_LOC
#define SHARED_MEMORY_SIZE (shared_mems_size[NUM_TILES]) /* LEGACY; should be called TILE_SHARED_SIZE */
#define TILE_SHARED_SIZE SHARED_MEMORY_SIZE
#define TILE_SHARED_START_USER_SPACE (TILE_SHARED_START + sizeof(shared_memory_map))

#define GLOBAL_TIMER 0x08FC0000
#define WALL_TIMER GLOBAL_TIMER
#define PARTITION_TIMER 0x08FD0000
#define VIRTUAL_TIMER PARTITION_TIMER

/***** programmatically retrieve the memory map *****/

#define NOT_SHARED ((uint32_t) (~0))

// names of all shared memories
// the tile memories are listed first, i.e. shared_mems[x] is idmem of tilex
extern const char shared_mems[NUM_SHARED_MEMORIES][20];
// base address of memory from a tile (-1 if not)
// (before address remapping) all memories have the same physical address for all tiles
extern const uint32_t shared_mems_start[NUM_SHARED_MEMORIES];
// size of all shared memories
extern const uint32_t shared_mems_size[NUM_SHARED_MEMORIES];

extern int get_shared_mem_index(char const * const mem);
extern int32_t get_shared_mem_start(char const * const mem);
extern int32_t get_shared_mem_size(char const * const mem);

#endif
