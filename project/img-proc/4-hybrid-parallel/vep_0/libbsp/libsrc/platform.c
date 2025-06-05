#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "platform.h"
#include "xil_printf.h"

// IMPORTANT: the tile memories must be listed first, i.e. shared_mems[x] is idmem of tilex
// this is assumed in generate-jason.c

// list of all shared memories
const char shared_mems[NUM_SHARED_MEMORIES][20] = { "mem0", "mem1", "mem2", "memshared0" };
// list of memories remotely reachable via the ring from a tile at a physical base address (-1 if not)
// NB tile i's memory is reachable locally too in a single cycle
// NB the first part of the shared memory is reserved for the system; user space starts at TILE_SHARED_START_USER_SPACE
const uint32_t shared_mems_start[NUM_SHARED_MEMORIES] = { TILE0_IDMEM_START, TILE1_IDMEM_START, TILE2_IDMEM_START, TILE_SHARED_START, };
const uint32_t shared_mems_size[NUM_SHARED_MEMORIES] = { 128*1024, 128*1024, 128*1024, 128*1024, };

int get_shared_mem_index(char const * const mem)
{
  if (mem == NULL) return -1;
  for (int i=0; i < NUM_SHARED_MEMORIES; i++)
    if (!strcmp(shared_mems[i], mem)) return i;
  return -1;
}

// get the starting address of the shared memory when accessing from tile t
// returns -1 if the memory doesn't exist or isn't reachable from that tile
int32_t get_shared_mem_start(char const * const mem)
{
  if (mem == NULL) return -1;
  for (uint32_t i=0; i < NUM_SHARED_MEMORIES; i++)
    if (!strcmp(shared_mems[i], mem)) return shared_mems_start[i];
  return -1;
}

// get the size the shared memory
// returns -1 if the memory doesn't exist
int32_t get_shared_mem_size(char const * const mem)
{
  if (mem == NULL) return -1;
  for (uint32_t i=0; i < NUM_SHARED_MEMORIES; i++)
    if (!strcmp(shared_mems[i], mem)) return shared_mems_size[i];
  return -1;
}