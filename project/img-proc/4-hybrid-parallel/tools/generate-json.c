#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <platform.h>
#include <cheapout.h>

// max size of vep-config.txt input file
#define MAXLINES 1000 /* 20 VEPs with max 50 lines each */
#define LINELENGTH 250

int power2(unsigned int i)
{
  unsigned int m = 1;
  while (m && i > m) m = m << 1;
  return (i == m);
}

int my_get_shared_mem_index(const char * const mem)
{
  if (mem == NULL) return -1;
  for (int i=0; i < NUM_SHARED_MEMORIES; i++)
    if (!strcmp(shared_mems[i], mem)) return i;
  return -1;
}

/* WARNING
 * NUM_PARTITIONS is used differently from other NUM_* constants
 * NUM_PARTITIONS is 4, meaning that there are 4 user partitions numbered from 1 to 4
 * PARTITION 0 is the system partition, and it must always be present in TDM slot 1 and @ 0 in instr/data memory
 * map->vps[] only contains user partitions
 */

typedef struct ranges {
  uint32_t start;
  uint32_t mask;
} ranges_t;

ranges_t *masks_list_global = NULL;
uint32_t nr_masks_global = 0;

int find_mask(uint32_t t) {
  uint32_t c = t;
  uint8_t i = 0;
  for (i = 0; i < 32; i++) {
    if ((c & 1) != 0) {
      break;
    }
    c >>= 1;
  }
  return ((uint32_t)(pow(2, i) - 1));
}

void find_args(uint32_t start, uint32_t end) {
  uint32_t start2 = start;
  if (start2 < end) {
    uint32_t lowermask = find_mask(start2);

    if ((start2 + lowermask + 1) > end) {
      lowermask = find_mask(end);

      masks_list_global = realloc(masks_list_global, (nr_masks_global + 1) * sizeof(ranges_t));
      masks_list_global[nr_masks_global].start = start2;
      masks_list_global[nr_masks_global].mask = ~lowermask;

      nr_masks_global++;
    } else {
      masks_list_global = realloc(masks_list_global, (nr_masks_global + 1) * sizeof(ranges_t));
      masks_list_global[nr_masks_global].start = start2;
      masks_list_global[nr_masks_global].mask = ~lowermask;
      nr_masks_global++;
    }
    start2 += lowermask + 1;
    find_args(start2, end);
  }
}
void sort() {
  int changed = 0;
  do {
    changed = 0;
    for (uint32_t i = 0; i < nr_masks_global - 1; i++) {
      if (masks_list_global[i].start > masks_list_global[i + 1].start) {
        ranges_t tmp = masks_list_global[i + 1];
        masks_list_global[i + 1] = masks_list_global[i];
        masks_list_global[i] = tmp;
        changed = 1;
      }
    }
  } while (changed);

  while (nr_masks_global > 0 && masks_list_global[0].start == 0) {
    masks_list_global++;
    nr_masks_global--;
  }
}

static int IsPowerOfTwo(uint32_t x)
{
  return (x != 0) && ((x & (x - 1)) == 0);
}

uint32_t compute_mask(uint32_t start, uint32_t length)
{
  // free array of previous run
  if (nr_masks_global != 0) {
    nr_masks_global = 0;
    //free(masks_list_global);
    masks_list_global = NULL;
  }

  uint32_t end = start + length;
  find_args(start, end);
  if (0) {
    fprintf(stderr, "-----------------------\n");
    fprintf(stderr, "start=%08X length=%08X\n", start, length);
    for (uint32_t i = 0; i < nr_masks_global; i++) {
      fprintf(stderr,"%08X %08X\n", masks_list_global[i].start, masks_list_global[i].mask);
    }
  }
  int changed = 0;
  do {
    changed = 0;
    for (uint32_t i = 0; i < nr_masks_global; i++) {
      for (uint32_t j = i + 1; j < nr_masks_global; j++) {
        if (masks_list_global[j].start != 0) {
          if (masks_list_global[i].mask == masks_list_global[j].mask) {
            uint32_t x = (~masks_list_global[i].start & masks_list_global[j].start);
            // one bit at the time.
            if (IsPowerOfTwo(x)) {
              if ((x | masks_list_global[i].start) == masks_list_global[j].start) {
                masks_list_global[i].mask &= ~x;
                masks_list_global[j].start = 0;
                masks_list_global[j].mask = 0;
                changed = 1;
              }
            }
          }
        }
      }
    }
    sort();
  } while (changed);

  if (0) {
    fprintf(stderr, "-----------------------\n");
    for (uint32_t i = 0; i < nr_masks_global; i++) {
      fprintf(stderr, "%08X %08X\n", masks_list_global[i].start, masks_list_global[i].mask);
    }
    fprintf(stderr, "---------- validate\n");
    // validating is very slow, hence commented out
    uint32_t lowermask = find_mask(start);
    uint32_t lowermask2 = find_mask(end);
    if (lowermask2 < lowermask)
      lowermask = lowermask2;
    if (0) fprintf(stderr, "set size: %u\n", lowermask + 1);
    for (uint64_t s = 0; s < UINT32_MAX; s += (lowermask + 1)) {
      int found = 0;
      for (uint32_t i = 0; i < nr_masks_global; i++) {
        if ((s & masks_list_global[i].mask) == masks_list_global[i].start) {
          found = 1;
        }
      }
      if ((s >= start && s < (start + length))) {
        if (found == 0) {
          fprintf(stderr,"internal error: cannot compute mask: %08X is not matched\n", (uint32_t)s);
          return 0;
        }
      } else {
        if (found == 1) {
          fprintf(stderr,"internal error: cannot compute mask: %08X is matched\n", (uint32_t)s);
          return 0;
        }
      }
      //printf("%08X\r", (uint32_t)s);
    }
  }
  return 1;
}

#define USAGE \
  fprintf(stderr, "Usage: %s [-debug] <path-to-vep-config.txt> -ld vep tile partition | -shmem vep | -json [-mm] [-tdm] [suspend vep tile partition ...]\n" , pgm);

int main(int argc , char *argv[]) {
  const char * const pgm = argv[0];
  if (argc < 2) {
    USAGE;
    return 1;
  }
  int debug = 0;
  int printmm = 0;
  int printtdm = 0;
  int json = 0;
  int ld_vep = -1, ld_tile = -1, ld_partition = -1;
  int mem_vep = -1;
  argc--;
  argv++;
  if (argc > 1 && !strcmp(argv[0], "-debug")) {
    debug = 1;
    argc--;
    argv++;
  }
  const char * const file = argv[0];
  argc--;
  argv++;
  if (argc >= 4 && !strcmp(argv[0], "-ld")) {
    // generate lscript for this partition
    ld_vep = atoi(argv[1]);
    ld_tile = atoi(argv[2]);
    ld_partition = atoi(argv[3]);
    if (ld_vep < 1 || ld_tile < 0 || ld_tile >= NUM_TILES || ld_partition < 1 || ld_partition > NUM_PARTITIONS) {
      USAGE;
      return 1;
    }
    argc -= 4;
    argv += 4;
  }
  if (argc >= 2 && !strcmp(argv[0], "-shmem")) {
    mem_vep = atoi(argv[1]);
    if (mem_vep < 1) {
      USAGE;
      return 1;
    }
    argc -= 4;
    argv += 4;
  }
  // should add a "suspend vep" command, -sp / -sv
  if (argc > 0 && !strcmp(argv[0], "-json")) {
    json = 1;
    argc--;
    argv++;
  }
  if (argc > 0 && !strcmp(argv[0], "-mm")) {
    printmm = 1;
    argc--;
    argv++;
  }
  if (argc > 0 && !strcmp(argv[0], "-tdm")) {
    printtdm = 1;
    argc--;
    argv++;
  }
  // next argument, if any must be "suspend"; it's parsed later on
  if (argc > 0 && strcmp(argv[0], "suspend")) {
    USAGE;
    return 1;
  }

  if ((json && ld_vep != -1) || (json && mem_vep != -1) || (!json && ld_vep != -1 && mem_vep != -1)) {
    fprintf(stderr, "%s: error: must have either -ld or -shmem or -json option\n", pgm);
    USAGE;
    return 1;
  }

  FILE *vc = fopen(file, "r");
  if (vc == NULL) {
    fprintf(stderr, "%s: error: cannot open %s\n" , pgm, file);
    return 1;
  }

  // read file into array
  char lines[MAXLINES][LINELENGTH] = { { 0 }, };
  int line = 0, ch = 0;
  while (!feof(vc) && line < MAXLINES && ch < LINELENGTH-1) {
    lines[line][ch] = fgetc(vc);
    if (lines[line][ch++] == '\n') { lines[line][ch] = '\0'; line++; ch = 0; }
  }
  if (!feof(vc) && line == MAXLINES) {
    fprintf(stderr, "%s: internal error: more than %d input lines\n", pgm, MAXLINES);
    return 1;
  }
  fclose(vc);

  if (0) {
    for (int i=0; i < line; i++) {
      fprintf(stderr,"%s", &lines[i][0]);
    }
  }

  unsigned int slot[NUM_TILES] = { 0 }; // nr slots in use on tile
  unsigned int in_vep[NUM_TILES][NUM_PARTITIONS+1] = { 0 };
  unsigned int alloc_slot[NUM_TILES][NUM_SLOTS+1] = { 0 };
  unsigned int alloc_slot_start[NUM_TILES][NUM_SLOTS+1] = { 0 };
  unsigned int alloc_slot_length[NUM_TILES][NUM_SLOTS+1] = { 0 };
  unsigned int alloc_period[NUM_TILES] = { 0 };
  unsigned int alloc_idmem[NUM_TILES][NUM_PARTITIONS+1] = { 0 };
  unsigned int alloc_idmem_start[NUM_TILES][NUM_PARTITIONS+1] = { 0 };
  unsigned int alloc_stack[NUM_TILES][NUM_PARTITIONS+1] = { 0 };
  // note that the first NUM_TILES shared memories are the tile memories
  // for each partition can share part of its allocated idmem (use second index 0..NUM_PARTITIONS)
  // alloc_shared_mem_start is computed, since always place the region at the end of the mem (above the stack)
  // the remainder NUM_TILES..NUM_SHARED_MEMORIES are the shared memories
  // alloc_shared_mem_start is specified in the vep-config.txt
  // there can be only one shared region per vep in the shared memories (use second index 0..NUM_VEPS+1)
  // NUM_VEPS >= NUM_PARTITIONS, so this is safe
  unsigned int alloc_shared_mem[NUM_SHARED_MEMORIES][NUM_VEPS+1] = { 0 };
  unsigned int alloc_shared_mem_start[NUM_SHARED_MEMORIES][NUM_VEPS+1] = { 0 };
  if (NUM_VEPS < NUM_PARTITIONS) {
    fprintf(stderr,"internal error: NUM_VEPS < NUM_PARTITIONS\n");
    exit(EXIT_FAILURE);
  }

  // allocate resources for the system application
  for (int t=0; t < NUM_TILES; t++) {
    slot[t] = 1;
    alloc_idmem[t][0] = SYS_APP_MEMORY*1024;
    alloc_idmem_start[t][0] = 0;
    alloc_stack[t][0] = 1; // not used, but must be > 0
    alloc_slot[t][0] = 0;
    alloc_slot_start[t][0] = 0; // assume that we start with an application slot, followed by a VKERNEL slot
    alloc_slot_length[t][0] = SYS_APP_CYCLES;
  }

  int error = 0, slot_list = -1;
  for (int i=0; i < line; i++) {
    unsigned int tile = 0, partition = 0, stack = DEFAULT_APP_STACK, cycles = DEFAULT_APP_SLOT_CYCLES, memory = 0, start = 0, period = 0, vp = 0;
    int check_mem = 0, check_shared_partition_mem = 0, check_shared_shared_mem = 0, check_slot_list = 0, check_slot_abs = 0, read = 0;
    char mem_string[LINELENGTH] = { '\0', } , s[LINELENGTH];
    // remove comments
    for (int j=0; lines[i][j] != '\0'; j++) if (lines[i][j] == '#') { lines[i][j++] = '\n'; lines[i][j] = '\0'; break; }
#define ECHO 0
    if (((read = sscanf(&lines[i][0], "vep %u on tile %u partition %u shares %uKiB private memory", &vp, &tile, &partition, &memory)) == 4) ||
        ((read = sscanf(&lines[i][0], "vep %u on tile %u partition %u shares 0x%xKiB private memory", &vp, &tile, &partition, &memory)) == 4)) {
      if (ECHO) fprintf(stderr,"> vep %u on tile %u partition %u shares %uKiB private memory\n", vp, tile, partition, memory);
      memory *= 1024;
      check_shared_partition_mem = 1;
    } else if (((read = sscanf(&lines[i][0], "vep %u in shared memory %s shares %uKiB private memory starting at %uKiB", &vp, mem_string, &memory, &start)) == 4) ||
        ((read = sscanf(&lines[i][0], "vep %u in shared memory %s shares 0x%xKiB private memory starting at 0x%xKiB", &vp, mem_string, &memory, &start)) == 4)) {
      if (ECHO) fprintf(stderr,"> vep %u in shared memory %s shares %uKiB private memory starting at %u\n", vp, mem_string, memory, start);
      memory *= 1024;
      start *= 1024;
      check_shared_shared_mem = 1;
    } else if ((read = sscanf(&lines[i][0], "vep %u on tile %u partition %u has %uKiB stack in %uKiB memory starting at %uKiB", &vp, &tile, &partition, &stack, &memory, &start)) == 6) {
      if (ECHO) fprintf(stderr,"> vep %u on tile %u partition %u has %uK stack in %uKiB memory starting at %uKiB\n", vp, tile, partition, stack, memory, start);
      memory *= 1024;
      start *= 1024;
      stack *= 1024;
      check_mem = 1;
    } else if ((read = sscanf(&lines[i][0], "vep %u on tile %u next slot is for partition %u with %u cycles", &vp, &tile, &partition, &cycles)) == 4) {
      // keep for testing since corresponds directly to how the TDM table is actually programmed
      if (ECHO) fprintf(stderr,"> vep %u on tile %u next slot is for partition %u with %u cycles\n", vp, tile, partition, cycles);
      check_slot_list = 1;
      if (slot_list == -1 || slot_list == 1) slot_list = 1;
      else {
        fprintf(stderr,"error: cannot mix different slot specifications: %s", &lines[i][0]);
        error =  1;
      }
    } else if ((read = sscanf(&lines[i][0], "vep %u on tile %u partition %u has %u cycles of %u starting at %u", &vp, &tile, &partition, &cycles, &period, &start)) == 6) {
      if (ECHO) fprintf(stderr,"> vep %u on tile %u slot partition %u has %u cycles of %u starting at %u\n", vp, tile, partition, cycles, period, start);
      check_slot_abs = 1;
      if (slot_list == -1 || slot_list == 0) slot_list = 0;
      else {
        fprintf(stderr,"error: cannot mix different slot specifications: %s", &lines[i][0]);
        error =  1;
      }
    } else if ((read = sscanf(&lines[i][0], "vep %u %s", &vp, s)) == 2) {
      if (ECHO) fprintf(stderr,"> vep %u %s\n", vp, s);
      fprintf(stderr, "error: cannot parse: %s", &lines[i][0]);
      error = 1;
    } else if ((read = sscanf(&lines[i][0], "vep %u", &vp)) == 1) {
      // skip blank line
      if (ECHO) fprintf(stderr,"> vep %u\n", vp);
    } else {
      fprintf(stderr, "error: cannot parse: %s", &lines[i][0]);
      exit(EXIT_FAILURE);
    }
    if (vp == 0 || vp >= NUM_VEPS) { fprintf(stderr, "error: vep (%d) must between 1 and %d: %s", vp, NUM_VEPS, &lines[i][0]); error = 1; }
    if (tile >= NUM_TILES) { fprintf(stderr, "error: tile must between 0 and %d: %s", NUM_TILES, &lines[i][0]); error = 1; }
    if (partition > NUM_PARTITIONS) { fprintf(stderr, "error: partition must between 1 and %d (inclusive): %s", NUM_PARTITIONS, &lines[i][0]); error = 1; }
    if (check_mem) {
      if (memory != 8*1024 && memory != 16*1024 && memory != 32*1024 && memory != 64*1024) { fprintf(stderr, "error: memory must be 4KiB, 8KiB, 16KiB, 32KiB, or 64KiB: %s", &lines[i][0]); error = 1; }
      else if (shared_mems_size[tile] % memory != 0 || start < 32*1024) { fprintf(stderr, "error: start must divide memory size (%dKiB) and not be less than 32KiB: %s", shared_mems_size[tile]/1024, &lines[i][0]); error = 1; }
      else if (start + memory > shared_mems_size[tile]) { fprintf(stderr, "error: memory start (%dKiB) + allocation (%dKiB) is larger than the memory size (%dKiB): %s", start/1024, memory/1024, shared_mems_size[tile]/1024, &lines[i][0]); error = 1; }
      else if (start % memory != 0) { fprintf(stderr, "error: start (%dKiB) must be a multiple (>=1) of size (%dKiB): %s", start/1024, memory/1024, &lines[i][0]); error = 1; }
      else if (stack >= memory) { fprintf(stderr, "error: stack must less than the memory (and leave space for code too!): %s", &lines[i][0]); error = 1; }
      else if (alloc_idmem[tile][partition] != 0) { fprintf(stderr, "error: memory already allocated: %s", &lines[i][0]); error = 1; }
      if (!error && partition != 0) {
        // memory & stack of system application cannot be changed by user
        alloc_idmem[tile][partition] = memory;
        alloc_idmem_start[tile][partition] = start;
        alloc_stack[tile][partition] = stack;
      }
      in_vep[tile][partition] = vp;
    }
    if (check_slot_list) {
      if (slot[tile] >= NUM_SLOTS-1) { fprintf(stderr, "error: maximum number of slots per tile is %d: %s", NUM_SLOTS-1, &lines[i][0]); error = 1; }
      else if (cycles <= 0) { fprintf(stderr, "error: cycles must be greater than 0: %s", &lines[i][0]); error = 1; }
      else if (alloc_slot[tile][slot[tile]] != 0) { fprintf(stderr, "error: slot already allocated: %s", &lines[i][0]); error = 1; }
      else {
        // user is allowed to allocate extra slots for the system application
        alloc_slot[tile][slot[tile]] = partition;
        alloc_slot_length[tile][slot[tile]] = cycles;
        slot[tile]++;
        in_vep[tile][partition] = vp;
      }
    }
    if (check_slot_abs) {
      if (slot[tile] >= NUM_SLOTS-1) { fprintf(stderr, "error: maximum number of TDM slots is %d: %s", NUM_SLOTS-1, &lines[i][0]); error = 1; }
      else if (period > NUM_TDM_CYCLES) { fprintf(stderr, "error: maximum TDM period is %d cycles: %s", NUM_TDM_CYCLES, &lines[i][0]); error = 1; }
      else if (cycles > period - 2*VKERNEL_CYCLES - SYS_APP_CYCLES ||
          start > period - cycles - VKERNEL_CYCLES) { fprintf(stderr, "error: period is too small: %s", &lines[i][0]); error = 1; }
      else if (alloc_period[tile] != 0 && alloc_period[tile] != period) { fprintf(stderr, "error: must have 1 period per tile (%d != %d): %s", alloc_period[tile], period, &lines[i][0]); error = 1; }
      else if (alloc_slot[tile][slot[tile]] != 0) { fprintf(stderr, "error: slot already allocated: %s", &lines[i][0]); error = 1; }
      else {
        // user is allowed to allocate extra slots for the system application
        alloc_slot[tile][slot[tile]] = partition;
        alloc_slot_start[tile][slot[tile]] = start;
        alloc_slot_length[tile][slot[tile]] = cycles;
        alloc_period[tile] = period;
        slot[tile]++;
        in_vep[tile][partition] = vp;
      }
    }
    if (check_shared_partition_mem) {
      if (start + memory > shared_mems_size[tile]) { fprintf(stderr, "error: allocation outside shared memory range: %s", &lines[i][0]); error = 1; }
      else if (partition == 0) { fprintf(stderr, "error: partition 0 cannot share memory: %s", &lines[i][0]); error = 1; }
      else if (alloc_shared_mem[tile][partition] != 0) { fprintf(stderr, "error: shared memory region already allocated: %s", &lines[i][0]); error = 1; }
      else {
        alloc_shared_mem[tile][partition] = memory;
        // cannot compute alloc_shared_mem_start yet
        in_vep[tile][partition] = vp;
      }
    }
    if (check_shared_shared_mem) {
      int mem_index = my_get_shared_mem_index(mem_string);
      if (mem_index == -1) { fprintf(stderr, "error: invalid shared memory: %s", &lines[i][0]); error = 1; }
      // shared memory with reserved system space -- see the mmu-add code below
      if (!error && mem_index == NUM_TILES && start < sizeof(shared_memory_map)) {
        fprintf(stderr, "error: allocation inside reserved shared memory range (0x0-0x%08X): %s", sizeof(shared_memory_map), &lines[i][0]);
        error = 1;
      }
      if (!error && start + memory > shared_mems_size[mem_index]) { fprintf(stderr, "error: allocation outside shared memory range: %s", &lines[i][0]); error = 1; }
      if (!error) {
        for (unsigned int p=1; p <= NUM_PARTITIONS; p++) {
          if (alloc_shared_mem[mem_index][vp] != 0) {
            fprintf(stderr, "error: shared memory region already allocated: %s", &lines[i][0]);
            error = 1;
            //break;
          }
        }
        if (!error) {
          alloc_shared_mem[mem_index][vp] = memory;
          alloc_shared_mem_start[mem_index][vp] = start;
        }
      }
    }
  }
  for (int t=0; t < NUM_TILES; t++) {
    for (unsigned int s=0; s < slot[t]; s++) {
      if(alloc_slot[t][s] && !alloc_idmem[t][alloc_slot[t][s]]) {
        fprintf(stderr, "error: partition %d on tile %d has slots but no memory allocated\n", alloc_slot[t][s], t);
        error = 1;
      }
    }
  }
  for (int t=0; t < NUM_TILES; t++) {
    unsigned int sum = 0;
    for (int p=0; p <= NUM_PARTITIONS; p++) {
      sum += alloc_idmem[t][p];
      if (!alloc_idmem[t][p]) continue;
      for (int p2=0; p2 < p; p2++) {
        if (!alloc_idmem[t][p2]) continue;
        if ((alloc_idmem_start[t][p] >= alloc_idmem_start[t][p2] &&
              alloc_idmem_start[t][p] < alloc_idmem_start[t][p2] + alloc_idmem[t][p2]) ||
            (alloc_idmem_start[t][p] + alloc_idmem[t][p] > alloc_idmem_start[t][p2] &&
             alloc_idmem_start[t][p] + alloc_idmem[t][p] < alloc_idmem_start[t][p2] + alloc_idmem[t][p2])) {
          fprintf(stderr, "error: partitions %d and %d have overlapping allocations in instruction/data memory on tile %u (%uKiB-%uKiB, %uKiB-%uKiB)\n",
              p, p2, t,
              alloc_idmem_start[t][p]/1024, (alloc_idmem_start[t][p] + alloc_idmem[t][p])/1024,
              alloc_idmem_start[t][p2]/1024, (alloc_idmem_start[t][p2] + alloc_idmem[t][p2])/1024);
          error = 1;
        }
      }
    }
    if (sum > shared_mems_size[t]) {
      fprintf(stderr, "error: too much memory (%dKiB) allocated to memory %s (%dKiB)\n", sum, shared_mems[t], shared_mems_size[t]/1024);
      error = 1;
    }
  }

  for (int t=0; t < NUM_TILES; t++) {
    for (int p=1; p <= NUM_VEPS; p++) {
      if (alloc_shared_mem[t][p] == 0) continue;
      if (alloc_idmem[t][p] == 0 && alloc_shared_mem[t][p] != 0) {
        fprintf(stderr, "error: partition %u on tile %u cannot share %uKiB partition memory without a memory allocation (%s)\n",
            p, t, alloc_shared_mem[t][p]/1024, shared_mems[t]);
        error = 1;
      }
      else if (alloc_idmem[t][p] != 0 && alloc_shared_mem[t][p] != 0 && alloc_idmem[t][p] - alloc_stack[t][p] < alloc_shared_mem[t][p]) {
        fprintf(stderr, "error: partition %u on tile %u shares more memory (%uKiB) than is available (%uKiB - %uKiB stack) in memory (%s)\n",
            p, t, alloc_shared_mem[t][p]/1024, alloc_idmem[t][p]/1024, alloc_stack[t][p]/1024, shared_mems[t]);
        error = 1;
      }
      else if (alloc_idmem[t][p] != 0 && alloc_shared_mem[t][p] != 0) {
        // place at the end of memory, after the stack; NB start is within the partition's idmem
        alloc_shared_mem_start[t][p] = alloc_idmem[t][p] - alloc_shared_mem[t][p];
      }
    }
  }

  // partition shared memory regions cannot overlap
  // but need to check that vep shared memory regions of different veps don't overlap
  for (int m=NUM_TILES; m < NUM_SHARED_MEMORIES; m++) {
    for (int v1=1; v1 <= NUM_VEPS; v1++) {
      for (int v2=1; v2 <= NUM_VEPS; v2++) {
        if (v1 == v2) continue;
        if (alloc_shared_mem_start[m][v1] >= alloc_shared_mem_start[m][v2] &&
            alloc_shared_mem_start[m][v1] < alloc_shared_mem_start[m][v2] + alloc_shared_mem[m][v2]) {
          fprintf(stderr, "error: shared regions of partitions %u and %u in memory %s overlap; start2=0x%08X <= start1=0x%08X < start2+end2=0x%08X\n",
              v1, v2, shared_mems[m], alloc_shared_mem_start[m][v2],
              alloc_shared_mem_start[m][v1], alloc_shared_mem_start[m][v2] + alloc_shared_mem[m][v2]);
          error = 1;
        }
        if (alloc_shared_mem_start[m][v1] + alloc_shared_mem[m][v1]-1 >= alloc_shared_mem_start[m][v2] &&
            alloc_shared_mem_start[m][v1] + alloc_shared_mem[m][v1]-1 < alloc_shared_mem_start[m][v2] + alloc_shared_mem[m][v2]) {
          fprintf(stderr, "error: shared regions of partitions %u and %u in memory %s overlap; start2=0x%08X <= start1+end1=0x%08X < start2+end2=0x%08X\n",
              v1, v2, shared_mems[m], alloc_shared_mem_start[m][v2],
              alloc_shared_mem_start[m][v1] + alloc_shared_mem[m][v1], alloc_shared_mem_start[m][v2] + alloc_shared_mem[m][v2]);
          error = 1;
        }
      }
    }
  }

  if (debug) {
    for (int t=0; t < NUM_TILES; t++) {
      for (int p=0; p <= NUM_PARTITIONS; p++) {
        if (in_vep[t][p]) fprintf(stderr, "in_vep[%d][%d]=%d\n", t, p, in_vep[t][p]);
      }
    }
    for (int t=0; t < NUM_TILES; t++) {
      for (int p=0; p <= NUM_PARTITIONS; p++) {
        if (alloc_idmem[t][p]) fprintf(stderr, "alloc_idmem[%d][%d]=%dKiB\n", t, p, alloc_idmem[t][p]/1024);
      }
    }
    for (int t=0; t < NUM_TILES; t++) {
      for (int p=0; p <= NUM_PARTITIONS; p++) {
        if (alloc_idmem_start[t][p]) fprintf(stderr, "alloc_idmem_start[%d][%d]=%dKiB\n", t, p, alloc_idmem_start[t][p]/1024);
      }
    }
    for (int t=0; t < NUM_TILES; t++) {
      for (int p=0; p <= NUM_PARTITIONS; p++) {
        if (alloc_stack[t][p]) fprintf(stderr, "alloc_stack[%d][%d]=%dKiB\n", t, p, alloc_stack[t][p]/1024);
      }
    }
    for (int t=0; t < NUM_SHARED_MEMORIES; t++) {
      for (int p=0; p <= NUM_VEPS; p++) {
        if (alloc_shared_mem[t][p]) fprintf(stderr, "alloc_shared_mem[%d][%d]=%dKiB\n", t, p, alloc_shared_mem[t][p]/1024);
      }
    }
    for (int t=0; t < NUM_SHARED_MEMORIES; t++) {
      for (int p=0; p <= NUM_VEPS; p++) {
        if (alloc_shared_mem_start[t][p]) fprintf(stderr, "alloc_shared_mem_start[%d][%d]=%dKiB\n", t, p, alloc_shared_mem_start[t][p]/1024);
      }
    }
    for (int t=0; t < NUM_TILES; t++) {
      for (unsigned int s=0; s < slot[t]; s++) {
        fprintf(stderr, "alloc_slot[%d][%d]=%d\n", t, s, alloc_slot[t][s]);
      }
    }
    if (slot_list == 0) {
      for (int t=0; t < NUM_TILES; t++) {
        for (unsigned int s=0; s < slot[t]; s++) {
          fprintf(stderr, "alloc_slot_start[%d][%d]=%d\n", t, s, alloc_slot_start[t][s]);
        }
      }
    }
    for (int t=0; t < NUM_TILES; t++) {
      for (unsigned int s=0; s < slot[t]; s++) {
        fprintf(stderr, "alloc_slot_length[%d][%d]=%d\n", t, s, alloc_slot_length[t][s]);
      }
    }
    for (int t=0; t < NUM_TILES; t++) {
      fprintf(stderr, "alloc_period[%d]=%d\n", t, alloc_period[t]);
    }
  }

  if (printmm || error) {
    for (int t=0; t < NUM_TILES; t++) {
      for (int p=1; p <= NUM_PARTITIONS; p++) {
        if (alloc_idmem[t][p] != 0) {
          fprintf(stderr, "on tile %d partition %d vep %2d map private instr/data region in %6s from 0x%08X = %3dKiB to 0x0; size %3dKiB = %3dKiB-%3dKiB\n",
              t, p, in_vep[t][p], shared_mems[t], 
              alloc_idmem_start[t][p],
              alloc_idmem_start[t][p]/1024,
              (alloc_idmem[t][p] - alloc_shared_mem[t][p])/1024,
              alloc_idmem[t][p]/1024, alloc_shared_mem[t][p]/1024);
        }
      }
    }
    for (int t=0; t < NUM_TILES; t++) {
      for (int p=1; p <= NUM_PARTITIONS; p++) {
        if (alloc_idmem[t][p] != 0) {
          for (int t1=0; t1 < NUM_TILES; t1++) {
            for (int p1=1; p1 <= NUM_PARTITIONS; p1++) {
              if (in_vep[t1][p1] == in_vep[t][p] && alloc_shared_mem[t1][p1] != 0) {
                fprintf(stderr, "on tile %d partition %d vep %2d map private shared region in %10s from 0x%08X to 0x%08X = base+%3dKiB+%3dKiB; size %3dKiB\n",
                    t, p, in_vep[t][p], shared_mems[t1], // we assume idmem of tile is ith mem; see platform.c
                    alloc_idmem_start[t1][p1] + alloc_shared_mem_start[t1][p1],
                    shared_mems_start[t1] + alloc_idmem_start[t1][p1] + alloc_shared_mem_start[t1][p1],
                    alloc_idmem_start[t1][p1]/1024, alloc_shared_mem_start[t1][p1]/1024, alloc_shared_mem[t1][p1]/1024);
              }
            }
          }
          for (int m=NUM_TILES; m < NUM_SHARED_MEMORIES; m++) {
            if (alloc_shared_mem[m][in_vep[t][p]] != 0) {
              fprintf(stderr, "on tile %d partition %d vep %2d map private shared region in %10s from 0x%08X to 0x%08X = base+%3dKiB       ; size %3dKiB\n",
                  t, p, in_vep[t][p], shared_mems[m], 
                  alloc_shared_mem_start[m][in_vep[t][p]],
                  shared_mems_start[m] + alloc_shared_mem_start[m][in_vep[t][p]],
                  alloc_shared_mem_start[m][in_vep[t][p]]/1024, alloc_shared_mem[m][in_vep[t][p]]/1024);
            }
          }
        }
      }
    }
  }

  if (error && !printtdm) {
    fprintf(stderr, "abort with memory allocation errors (printed before the memory map, which is provided for debugging)\n");
    exit(EXIT_FAILURE);
  }
  if (!error && printmm) return EXIT_SUCCESS;

  ///// generate slot list from absolute slots /////
  if (ld_tile == -1 && slot_list == 0) {
    // sort slots on starting time
    int alloc_slot2[NUM_TILES][NUM_SLOTS+1] = { 0 };
    int alloc_slot_start2[NUM_TILES][NUM_SLOTS+1] = { 0 };
    int alloc_slot_length2[NUM_TILES][NUM_SLOTS+1] = { 0 };
    for (int t=0; t < NUM_TILES; t++) {
      // temporarily add a slot to correspond to the specified period
      if (alloc_period[t] == 0) {
        // we didn't assigned any slots on this tile, only have one system application slot
        continue;
      }
      alloc_slot_start[t][slot[t]] = alloc_period[t];
      slot[t]++;
      unsigned int slots = 0;
      while (slots < slot[t]) {
        unsigned int smallest = ~0, si = -1;
        for (unsigned int s=0; s < slot[t]; s++) {
          if (alloc_slot_start[t][s] <= smallest) {
            smallest = alloc_slot_start[t][s];
            si = s;
          }
        }
        alloc_slot2[t][slots] = alloc_slot[t][si];
        alloc_slot_start2[t][slots] = alloc_slot_start[t][si];
        alloc_slot_length2[t][slots] = alloc_slot_length[t][si];
        alloc_slot_start[t][si] = ~0 -1;
        slots++;
      }
      // check that slot 0 is for system application
      // assume that we start with application slot, followed by a VKERNEL slot
      // check for overlapping slots & gaps, including the VKERNEL
      if (alloc_slot_start2[t][0] != 0) {
        fprintf(stderr, "error: tile %d slot 0 is misaligned (should start at %d cycles)\n", t, VKERNEL_CYCLES);
        error = 1;
      }
      if (alloc_slot2[t][0] != 0) {
        fprintf(stderr, "error: tile %d slot 0 isn't for the system application\n", t);
        error = 1;
      }
      for (unsigned int s=1; s < slots; s++) {
        int diff = alloc_slot_start2[t][s] - (alloc_slot_start2[t][s-1] + alloc_slot_length2[t][s-1] + VKERNEL_CYCLES);
        if (diff == 0) { /* nothing to do */ ; }
        else if (diff < 0) {
          fprintf(stderr, "error: tile %d slot %d finishes at %d which is after the next slot/period starts at %d\n",
              t, s, alloc_slot_start2[t][s-1] + alloc_slot_length2[t][s-1] + VKERNEL_CYCLES, alloc_slot_start2[t][s]);
          error = 1;
        }
        else if (diff < 2*VKERNEL_CYCLES) {
          // padding slot must be at least VKERNEL cycles (arbitrary choice)
          fprintf(stderr, "error: tile %d slot %d finishes at %d which is too close to the start of the next slot/period %d (must >= %d cycles)\n",
              t, s, alloc_slot_start2[t][s-1] + alloc_slot_length2[t][s-1] + VKERNEL_CYCLES, alloc_slot_start2[t][s], 2*VKERNEL_CYCLES);
          error = 1;
        }
        else if (slot[t] == NUM_SLOTS +1) { // +1 for temporary extra slot
          fprintf(stderr, "error: tile %d ran out of slots, cannot insert padding slot after slot %d\n", t, s);
          error = 1;
        }
        else {
          // insert padding slot for the system application, shifting everything up
          if (debug) fprintf(stderr, "info: insert padding tile %d slot %d from %d to %d\n",
              t, s, alloc_slot_start2[t][s-1] + alloc_slot_length2[t][s-1] + VKERNEL_CYCLES, alloc_slot_start2[t][s]);
          for (unsigned int x = slots; x >= s; x--) {
            alloc_slot2[t][x] = alloc_slot2[t][x-1];
            alloc_slot_start2[t][x] = alloc_slot_start2[t][x-1];
            alloc_slot_length2[t][x] = alloc_slot_length2[t][x-1];
          }
          alloc_slot2[t][s] = 0;
          alloc_slot_start2[t][s] = alloc_slot_start2[t][s-1] + alloc_slot_length2[t][s-1] + VKERNEL_CYCLES;
          alloc_slot_length2[t][s] = diff - VKERNEL_CYCLES;
          slots++;
          s--;
        }
      }
      for (unsigned int s=0; s < slots; s++) {
        alloc_slot[t][s] = alloc_slot2[t][s];
        alloc_slot_start[t][s] = alloc_slot_start2[t][s];
        alloc_slot_length[t][s] = alloc_slot_length2[t][s];
      }
      // remove the extra 'period' slot
      slot[t] = slots -1;
    }
    if (error || printtdm) {
      for (int t=0; t < NUM_TILES; t++) {
        unsigned int sum = 0;
        for (unsigned int s=0; s < slot[t]; s++) {
          fprintf(stderr, "tile %d slot %2d from %6d, length %6d, partition %d of vep %d\n",
              t, s, sum, alloc_slot_length[t][s], alloc_slot[t][s], in_vep[t][alloc_slot[t][s]]);
          sum += alloc_slot_length[t][s] + VKERNEL_CYCLES;
        }
      }
    }
  }

  if (error) {
    fprintf(stderr, "abort with TDM allocation errors (printed before the TDM overview, which is provided for debugging)\n");
    exit(EXIT_FAILURE);
  }

  ///// generate lscript.ld for partition ////

  if (ld_tile != -1) {
    if (alloc_idmem[ld_tile][ld_partition] == 0) {
      fprintf(stderr, "error: vep %d tile %d partition %d has no memory allocated; add it to vep-config.txt to use it or move/rename it to ignore it\n", ld_vep, ld_tile, ld_partition);
      fprintf(stderr, "abort with errors\n");
      exit(EXIT_FAILURE);
    }
    printf("/* DO NOT EDIT\n"
        " * this file was automatically generated by %s from ../vep-config.txt for\n"
        " * VEP_ID=%d TILE_ID=%d PARTITION_ID=%d\n"
        " */\n\n",
        pgm, ld_vep, ld_tile, ld_partition);
    printf("MEMORY {\n");
    printf("mem : ORIGIN = 0x0, LENGTH = 0x%08X /* remove shared partition region, starting after the stack at LENGTH */\n",
        alloc_idmem[ld_tile][ld_partition] - alloc_shared_mem[ld_tile][ld_partition]);
    for (int m=0; m < NUM_SHARED_MEMORIES; m++) {
      if (m < NUM_TILES) {
        // idmem shared by partition
        for (int p=1; p <= NUM_PARTITIONS; p++) {
          if (in_vep[m][p] == in_vep[ld_tile][ld_partition] && alloc_shared_mem[m][p] != 0) {
            // this is parsed by rerun.sh that needs to know the physical addresses of the private region to zero it
            printf("%s_partition%u : ORIGIN = 0x%08X, LENGTH = 0x%08X /* vep=%d tile=%d partition=%d private shared memory region */\n",
                shared_mems[m], p, // we assume idmem of tile is ith mem; see platform.c
                shared_mems_start[m] + alloc_idmem_start[m][p] + alloc_shared_mem_start[m][p],
                alloc_shared_mem[m][p],
                ld_vep, m, p);
          }
        }
      } else {
        // shared memory shared by vep
        unsigned int vep = in_vep[ld_tile][ld_partition];
        if (alloc_shared_mem[m][vep] != 0) {
          // this is parsed by rerun.sh that needs to know the physical addresses of the private region to zero it
          printf("%s : ORIGIN = 0x%08X, LENGTH = 0x%08X /* vep=%d private shared memory region */\n",
              shared_mems[m], shared_mems_start[m] + alloc_shared_mem_start[m][vep],
              alloc_shared_mem[m][vep], vep);
        }
      }
    }
    printf("}\n");
    printf("_STACK_SIZE = 0x%08X;\n", alloc_stack[ld_tile][ld_partition]);
    printf(
        "OUTPUT_ARCH( \"riscv\" )\n"
        "ENTRY( _start )\n"
        "_STACK_SIZE = DEFINED(_STACK_SIZE) ? _STACK_SIZE : 0x1000;\n"
        "SECTIONS\n"
        "{\n"
        "  . = 0x00000000;\n"
        "  .text.init : { *(.text.init) } > mem\n"
        "  .text : { *(.text) } > mem\n"
        "  .data : { *(.data) } > mem\n"
        "  .bss.align :\n"
        "   {\n"
        "           . = ALIGN(4);\n"
        "           _bss = .;\n"
        "   } > mem\n"
        "   .bss.start :\n"
        "   {\n"
        "          _bss_lma = LOADADDR(.bss.start);\n"
        "   } >mem\n"
        "   .bss :\n"
        "   {\n"
        "           *(.sbss*)\n"
        "           *(.gnu.linkonce.sb.*)\n"
        "           *(.bss .bss.*)\n"
        "           *(.gnu.linkonce.b.*)\n"
        "           *(COMMON)\n"
        "        . = ALIGN(4);\n"
        "        _ebss = .;\n"
        "   } >mem\n"
        );
    for (int m=0; m < NUM_SHARED_MEMORIES; m++) {
      if (m < NUM_TILES) {
        // idmem shared by partition
        for (int p=1; p <= NUM_PARTITIONS; p++) {
          if (in_vep[m][p] == in_vep[ld_tile][ld_partition] && alloc_shared_mem[m][p] != 0) {
            printf("  .%s_partition%u : { *(.%s_partition%u) } > %s_partition%u\n", shared_mems[m], p, shared_mems[m], p, shared_mems[m], p);
          }
        }
      } else {
        // shared memory shared by vep
        unsigned int vep = in_vep[ld_tile][ld_partition];
        if (alloc_shared_mem[m][vep] != 0) {
          printf("  .%s : { *(.%s) } > %s\n", shared_mems[m], shared_mems[m], shared_mems[m]);
        }
      }
    }
    printf(
        "  _end = .;\n"
        "  .stack (NOLOAD) : {\n"
        "          _stack_end = .;\n"
        "          . += _STACK_SIZE;\n"
        "          . = ALIGN(16);\n"
        "          _stack_start = .;\n"
        "  } > mem\n"
        "  PROVIDE (end = .);\n"
        "}\n"
        );

    ///// generate commands for dynload /////

    // WARNING: hardcoded file name
    FILE *dynl = fopen("dynload-clear.cmd", "w");
    if (dynl == NULL) {
      fprintf(stderr, "%s: error: cannot open dynload-clear.cmd\n", pgm);
      exit(EXIT_FAILURE);
    }
    // clear the shared region of this partition
    if (alloc_shared_mem[ld_tile][ld_partition] != 0)
      fprintf(dynl,"%u clear 0x%08X 0x%08X\n", ld_tile, alloc_idmem_start[ld_tile][ld_partition] + alloc_shared_mem_start[ld_tile][ld_partition], alloc_shared_mem[ld_tile][ld_partition]);
    fclose(dynl);

    // will be (over)written for each partition, but that's ok
    dynl = fopen("../shared_memories/dynload-clear.cmd", "w");
    if (dynl == NULL) {
      fprintf(stderr, "%s: error: cannot open ../shared/_memories/dynload-clear.cmd\n", pgm);
      exit(EXIT_FAILURE);
    }
    // clear the shared regions of this vep
    for (int m=NUM_TILES; m < NUM_SHARED_MEMORIES; m++) {
      if (alloc_shared_mem[m][ld_vep] != 0) {
        fprintf(dynl,"%u clear 0x%08X 0x%08X\n", ld_tile,
            shared_mems_start[m] + alloc_shared_mem_start[m][ld_vep], alloc_shared_mem[m][ld_vep]);
      }
    }
    fclose(dynl);
    return EXIT_SUCCESS;
  }

  ///// generate vep_memory_map.[ch] for ARM or RISC-V ////

  if (mem_vep != -1) {
    ///// generate memory.h file /////

    // WARNING: hardcoded file name
    // will be (over)written for each partition, but that's ok
    FILE *memc = fopen("../shared_memories/vep_memory_map.c", "w");
    if (memc == NULL) {
      fprintf(stderr, "%s: error: cannot open ../shared_memories/vep_memory_map.c\n" , pgm);
      exit(EXIT_FAILURE);
    }
    fprintf(memc,"/* DO NOT EDIT\n"
        " * this file was automatically generated by %s from vep-config.txt for VEP_ID=%d\n"
        " */\n\n", pgm, mem_vep);

    fprintf(memc,"#include <vep_memory_map.h>\n");
    fprintf(memc,"\n");
    // note that the VEP view on the shared memories uses a vep_memories_remote_start/size array
    // that has 0..NUM_PARTITIONS-1 entries, which correspond only to entries 1..NUM_PARTITIONS used in here
    // for the non-tile shared memories (memory # >= NUM_TILES), here we store the VEP data
    // but for the user, other VEPs are not relevant, and we store the partition addresses to reach the shared memory
    // all partition entries are the same, since they can all access the shared memories using the same adddress
    fprintf(memc,"uint32_t const vep_memories_shared_remote_start[NUM_SHARED_MEMORIES][NUM_PARTITIONS]= {\n");
    for (int m=0; m < NUM_SHARED_MEMORIES; m++) {
      fprintf(memc,"  { ");
      for (int p=1; p <= NUM_PARTITIONS; p++) {
        if (m < NUM_TILES) {
          if (alloc_shared_mem[m][p] == 0) fprintf(memc, "NOT_SHARED, ");
          else fprintf(memc,"0x%08X, ", shared_mems_start[m] + alloc_idmem_start[m][p] + alloc_shared_mem_start[m][p]);
        } else {
          if (alloc_shared_mem[m][mem_vep] == 0) fprintf(memc, "NOT_SHARED, ");
          else fprintf(memc,"0x%08X, ", shared_mems_start[m] + alloc_shared_mem_start[m][mem_vep]);
        }
      }
      fprintf(memc,"},\n");
    }
    fprintf(memc,"};\n");
    fprintf(memc,"uint32_t const vep_memories_shared_remote_size[NUM_SHARED_MEMORIES][NUM_PARTITIONS]= {\n");
    for (int m=0; m < NUM_SHARED_MEMORIES; m++) {
      fprintf(memc,"  { ");
      for (int p=1; p <= NUM_PARTITIONS; p++) {
        if (m < NUM_TILES) {
          if (alloc_shared_mem[m][p] == 0) fprintf(memc, "NOT_SHARED, ");
          else fprintf(memc,"0x%08X, ", alloc_shared_mem[m][p]);
        } else {
          if (alloc_shared_mem[m][mem_vep] == 0 ) fprintf(memc, "NOT_SHARED, ");
          else fprintf(memc,"0x%08X, ", alloc_shared_mem[m][mem_vep]);
        }
      }
      fprintf(memc,"},\n");
    }
    fprintf(memc,"};\n");
    fprintf(memc,"\n");
    // TODO ugly, fix later
    fprintf(memc,
        "#ifdef PARTITION_ID\n"
        "#include <xil_printf.h>\n"
        "#define print xil_printf\n"
        "#else\n"
        "#include <stdio.h>\n"
        "#define print printf\n"
        "#endif\n"
        "\n"
        "void print_vep_memory_map(void)\n"
        "{\n"
        "  // regions shared by partitions\n"
        "  for (uint32_t m=0; m < NUM_TILES; m++) {\n"
        "    for (uint32_t p=0; p < NUM_PARTITIONS; p++) {\n"
        "      if (vep_memories_shared_remote_start[m][p] != NOT_SHARED)\n"
        "        print(\"in memory %%s partition %%u of vep %%u shares region of %%uKiB starting at 0x%%08X\\n\",\n"
        "          shared_mems[m], p, VEP_ID, vep_memories_shared_remote_size[m][p]/1024, vep_memories_shared_remote_start[m][p]);\n"
        "    }\n"
        "  }\n"
        "  // region shared by the VEP\n"
        "  // no need for a partition loop since all partitions access the region at the same address\n"
        "  for (uint32_t m=NUM_TILES; m < NUM_SHARED_MEMORIES; m++) {\n"
        "    if (vep_memories_shared_remote_start[m][0] != NOT_SHARED)\n"
        "        print(\"in memory %%s vep %%u shares region of %%uKiB starting at 0x%%08X\\n\",\n"
        "          shared_mems[m], VEP_ID, vep_memories_shared_remote_size[m][0]/1024, vep_memories_shared_remote_start[m][0]);\n"
        "  }\n"
        "}\n"
        );
    fclose(memc);

    FILE *memh = fopen("../shared_memories/vep_memory_map.h", "w");
    if (memh == NULL) {
      fprintf(stderr, "%s: error: cannot open ../shared_memories/vep_memory_map.c\n" , pgm);
      exit(EXIT_FAILURE);
    }
    fprintf(memh,"/* DO NOT EDIT\n"
        " * this file was automatically generated by %s from vep-config.txt for VEP_ID=%d\n"
        " */\n\n", pgm, mem_vep);
    fprintf(memh,"#ifndef __VEP_MEMORY_MAP_H_\n");
    fprintf(memh,"#define __VEP_MEMORY_MAP_H_\n\n");
    fprintf(memh,"#include <stdint.h>\n");
    fprintf(memh,"#include <platform.h>\n");
    fprintf(memh,"\n");
    for (int m=NUM_TILES; m < NUM_SHARED_MEMORIES; m++) {
      if (alloc_shared_mem[m][mem_vep] != 0) {
        char s[100] = { '\0' };
        for (int l = 0; shared_mems[m][l] != '\0'; l++) s[l] = toupper(shared_mems[m][l]);
        fprintf(memh,"#define VEP_%s_SHARED_REGION_SIZE                0x%08X\n", s, alloc_shared_mem[m][mem_vep]);
        fprintf(memh,"#define VEP_%s_SHARED_REGION_REMOTE_START        0x%08X\n",
            s, shared_mems_start[m] + alloc_shared_mem_start[m][mem_vep]);
      }
    }
    for (int t=0; t < NUM_TILES; t++) {
      for (int p=1; p <= NUM_PARTITIONS; p++) {
        if (alloc_idmem[t][p] != 0) {
          fprintf(memh,"#define VEP_TILE%u_PARTITION%u_IDMEM_SIZE                  0x%08X%s\n",
                  t, p, alloc_idmem[t][p] - alloc_shared_mem[t][p],
                  (alloc_shared_mem[t][p] == 0 ? "" : " /* without shared region */"));
          fprintf(memh,"#define VEP_TILE%u_PARTITION%u_IDMEM_START                 0x00000000 /* always 0 */\n", t, p);
          if (alloc_shared_mem[t][p] != 0) {
            fprintf(memh,"#define VEP_TILE%u_PARTITION%u_SHARED_REGION_SIZE          0x%08X\n", t, p, alloc_shared_mem[t][p]);
            fprintf(memh,"#define VEP_TILE%u_PARTITION%u_SHARED_REGION_LOCAL_START   0x%08X /* only for partition_%u_%u */\n",
                    t, p, alloc_shared_mem_start[t][p], t, p);
            fprintf(memh,"#define VEP_TILE%u_PARTITION%u_SHARED_REGION_REMOTE_START  0x%08X\n",
                    t, p, shared_mems_start[t] + alloc_idmem_start[t][p] + alloc_shared_mem_start[t][p]);
          }
        }
      }
    }
    fprintf(memh,"\n");
    fprintf(memh,"// the following arrays contain the same information as the #defines\n");
    fprintf(memh,"// start/size of memory regions shared by partitions for memories 0..%d: ", NUM_TILES-1);
    for (int m=0; m < NUM_TILES; m++) fprintf(memh,"%s ",shared_mems[m]);
    fprintf(memh,"\n");
    fprintf(memh,"// and by the vep (i.e. same for all partitions) for the shared memories %d..%d: ", NUM_TILES, NUM_SHARED_MEMORIES-1);
    for (int m=NUM_TILES; m < NUM_SHARED_MEMORIES; m++) fprintf(memh,"%s ",shared_mems[m]);
    fprintf(memh,"\n");
    fprintf(memh,"// thus you can access tile t, partition p's shared region at vep_memories_shared_remote_start[t][p]\n");
    fprintf(memh,"// and the vep's shared region in memory m at vep_memories_shared_remote_start[m][p] (same for all partitions p)\n");
    fprintf(memh,"// note that the system partition 0 is excluded; shown are user partitions 1..%u\n", NUM_PARTITIONS);
    fprintf(memh,"extern uint32_t const vep_memories_shared_remote_start[NUM_SHARED_MEMORIES][NUM_PARTITIONS];\n");
    fprintf(memh,"extern uint32_t const vep_memories_shared_remote_size[NUM_SHARED_MEMORIES][NUM_PARTITIONS];\n");
    fprintf(memh,"extern void print_vep_memory_map(void);\n");
    fprintf(memh,"#endif\n");
    fclose(memh);
    return EXIT_SUCCESS;
  }

  if (json) {
    // see which applications to not schedule
    // cannot suspend application 0 (in any slot) on any tile
    int i = 1;
    unsigned int vep, tile, part;
    if (argc > 0 && argc < 4) error = 1;
    while (!error && i < argc-1) {
      if (sscanf(argv[i],"%u", &vep) == 1 && sscanf(argv[i+1],"%u", &tile) == 1 && sscanf(argv[i+2],"%u", &part) == 1) {
        if (vep == 0 || vep >= NUM_VEPS || tile >= NUM_TILES || part <= 0 || part > NUM_PARTITIONS) {
          fprintf(stderr, "error: invalid vep %u or tile %u or partition %u\n", vep, tile, part);
          error = 1;
          break;
        }
        int suspended = 0;
        for (unsigned int s=1; s < slot[tile]; s++) {
          if (alloc_slot[tile][s] == part) {
            fprintf(stderr, "info: suspend vep %u tile %u partition %u slot %u\n", vep, tile, part, s);
            alloc_slot[tile][s] = 0;
            suspended = 1;
          }
        }
        i += 3;
        if (!suspended)
          fprintf(stderr, "info: did not suspend vep %u tile %u partition %u\n", vep, tile, part);
      }
    }
    if (error || (argc > 2 && i != argc)) {
      USAGE;
      fprintf(stderr, "abort with errors\n");
      exit(EXIT_FAILURE);
    }

    // standard platform
    // the 10000000 doesn't matter (it's the total TDM length, it just needs to be large enough)
    printf(
        "tile-add    0 40000000 10000000 128k 32 16\n"
        "tile-add    1 40000000 10000000 128k 32 16\n"
        "tile-add    2 40000000 10000000 128k 32 16\n"
        "tile-tdm-os 0 %d\n"
        "tile-tdm-os 1 %d\n"
        "tile-tdm-os 2 %d\n"
        "tile-mem-os 0 %dk\n"
        "tile-mem-os 1 %dk\n"
        "tile-mem-os 2 %dk\n",
        VKERNEL_CYCLES, VKERNEL_CYCLES, VKERNEL_CYCLES,
        SYS_APP_MEMORY, SYS_APP_MEMORY, SYS_APP_MEMORY
        );

    int sum[NUM_TILES] = { 0 };

    for (int t=0; t < NUM_TILES; t++) {
      for (int p=1; p <= NUM_PARTITIONS; p++) {
        if (alloc_idmem[t][p] != 0) {
          int s = 0;
          // remove the shared private region, which is placed at the end of the partition's IDMEM
          printf("tile-mem-add  %d %d %dk 0x%X\n", t, p, alloc_idmem[t][p]/1024, alloc_idmem_start[t][p]);
          // we don't check if the vep directory actually exists; dynload will silently ignore -> TODO
          printf("tile-mem-data %d %d vep_%d/partition_%d_%d/out.hex\n", t, p, in_vep[t][p], t, p);
          // we don't lock the application in the mapping
          // note that this means that they may be moved around when adding new applications
          // when dynamically loading; there's currently no good way to deal with this
          //printf("tile-mem-lock %d %d\n", t, p);

          printf("# global & partition timers\n");
          printf("tile-mmu-add  %d %d %d 0x08FC0000 0x08FC0000 0xFFFEFFF8\n", t, p, s++);

          shared_memory_map *map = (shared_memory_map *) PLATFORM_OCM_MEMORY_LOC;
          printf("# ARM-RISCV stdin/out for system (vp_ios)\n");
          uint32_t mask = ~((sizeof(cheapout_t)) -1);
          printf("tile-mmu-add  %d %d %d 0x%08X 0x%08X 0x%0X\n",
              t, p, s++, PLATFORM_OCM_MEMORY_LOC, (unsigned int) &(map->vp_ios[t*NUM_PARTITIONS+p-1]), mask);
          printf("# ARM-RISCV stdout for the partition (vp_user)\n");
          mask = ~((sizeof(cheapout_user_t)) -1);
          printf("tile-mmu-add  %d %d %d 0x%08X 0x%08X 0x%08X\n",
              t, p, s++, PLATFORM_OCM_MEMORY_LOC + sizeof(cheapout_user_t), (unsigned int) &(map->vp_users[t*NUM_PARTITIONS+p-1]), mask);
          // user-accessible memory (for all apps) starts at TILE_SHARED_START_USER_SPACE == PLATFORM_OCM_MEMORY_LOC+sizeof(shared_memory_map)

          printf("# memory regions shared by the VEP in shared memories\n");
          for (int m=NUM_TILES; m < NUM_SHARED_MEMORIES; m++) {
            if (alloc_shared_mem[m][in_vep[t][p]] != 0) {
              if (!compute_mask(shared_mems_start[m] + alloc_shared_mem_start[m][in_vep[t][p]], alloc_shared_mem[m][in_vep[t][p]])) {
                fprintf(stderr, "internal error: cannot compute mask for: "
                    "mem %d vep %d shared_mems_start=0x%08X alloc_shared_mem_start=0x%08X alloc_shared_mem=0x%08X\n",
                    m, vep, shared_mems_start[m], alloc_shared_mem_start[m][in_vep[t][p]], alloc_shared_mem[m][in_vep[t][p]]);
                exit(EXIT_FAILURE);
              }
              for (uint32_t i=0; i < nr_masks_global; i++) {
                printf("tile-mmu-add  %d %d %d 0x%08X 0x%08X 0x%08X\n",
                    t, p, s++, masks_list_global[i].start, masks_list_global[i].start, masks_list_global[i].mask);
              }
            }
          }
          printf("# memory regions shared by partitions in instruction/data memories\n");
          for (int t1=0; t1 < NUM_TILES; t1++) {
            for (int p1=1; p1 <= NUM_PARTITIONS; p1++) {
              if (in_vep[t1][p1] == in_vep[t][p] && alloc_shared_mem[t1][p1] != 0) {
                if (!compute_mask(shared_mems_start[t1] + alloc_idmem_start[t1][p1] + alloc_shared_mem_start[t1][p1], alloc_shared_mem[t1][p1])) {
                  fprintf(stderr, "internal error: cannot compute mask for: "
                      "tile %d partition %d shared_mems_start=0x%08X alloc_idmem_start=0x%08X alloc_shared_mem_start=0x%08X alloc_shared_mem=0x%08X\n",
                      t, p, shared_mems_start[t1], alloc_idmem_start[t1][p1], alloc_shared_mem_start[t1][p1], alloc_shared_mem[t1][p1]);
                  exit(EXIT_FAILURE);
                }
                for (uint32_t i=0; i < nr_masks_global; i++) {
                  printf("tile-mmu-add  %d %d %d 0x%08X 0x%08X 0x%08X\n",
                      t, p, s++, masks_list_global[i].start, masks_list_global[i].start, masks_list_global[i].mask);
                  // open up the memory region but don't remap
                }
              }
              if (s >= NUM_MMU_ENTRIES) {
                fprintf(stderr, "internal error: maximum # MMU entries %d\n", s);
                exit(EXIT_FAILURE);
              }
            }
          }
        }
      }
      for (unsigned int s=0; s < slot[t]; s++) {
        sum[t] += alloc_slot_length[t][s] + VKERNEL_CYCLES;
        // old non-zero slots are cleared by dynload
        if (alloc_slot_length[t][s])
          printf("tile-tdm-add  %d %d %d\n", t, alloc_slot[t][s], alloc_slot_length[t][s]);
      }
    }
    fprintf(stderr, "info: slot table lengths are: ");
    for (int t=0; t < NUM_TILES; t++) fprintf(stderr,"%d ", sum[t]);
    fprintf(stderr,"cycles\n");
  }
  return EXIT_SUCCESS;
  }
