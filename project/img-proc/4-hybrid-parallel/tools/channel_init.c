#include <stdlib.h>
#include <errno.h>
#include <fcntl.h> //open
#include <unistd.h> //close
#include <sys/mman.h> //mmap
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "cheapout.h"



// Setup to use normal malloc and free
void * ( *_dynmalloc )( size_t ) = malloc;
void   ( *_dynfree )( void* )    = free;

int main( int argc, char **argv)
{
    int verbose = 1;
    if (argc == 2 && !strcmp(argv[1],"-lessinfo")) verbose = 0;

	int err = 0;
	if (verbose) printf("mem: 16 KB @ 0x%p\n", (void *)OCM_LOC[0]);

	//volatile uint32_t* buf;
	int memf = open("/dev/mem"
			, O_RDWR | O_SYNC //do I want cacheing?
		       );
	if(memf < 0) {
		err = errno;
		fprintf(stderr, "FAILED open %d\n", err);
		goto err_;
	}
	volatile void* ocm = mmap(NULL, sizeof(shared_memory_map), PROT_READ | PROT_WRITE, MAP_SHARED, memf,
			OCM_LOC[0]);
	if(ocm == MAP_FAILED) {
		err = errno;
		fprintf(stderr, "FAILED open %d\n", err);
		goto err_close;
	}

	shared_memory_map *map = (shared_memory_map*)ocm;
	shared_memory_map *remmap = (shared_memory_map*)PLATFORM_OCM_MEMORY_LOC;
	for(int i = 0; i < NUM_TILES; i++ ){
		cheapout_sys admin_sys = (cheapout_sys) &(map->vp_kernel[i]);
		cheapout_sys admin_sys_rem = (cheapout_sys) &(remmap->vp_kernel[i]);
		cheap admin = cheap_init((void*)&(admin_sys_rem->buffer_stdout), CHEAP_BUFFER_SIZE_STDIO, sizeof(uint8_t));
		cheap admin3 = cheap_init((void*)&(admin_sys_rem->buffer_user_out), CHEAP_BUFFER_SIZE_SYS, sizeof(uint8_t));
		cheap admin4 = cheap_init((void*)&(admin_sys_rem->buffer_user_in),  CHEAP_BUFFER_SIZE_SYS, sizeof(uint8_t));
		memcpy((void *) &(admin_sys->admin_stdout), admin, sizeof(cheap_t));
		memcpy((void *) &(admin_sys->admin_user_out),admin3, sizeof(cheap_t));
		memcpy((void *) &(admin_sys->admin_user_in), admin4, sizeof(cheap_t));
		if (verbose) printf("Kernel: %d\n", i);
	}
	for ( int i =0; i < (NUM_TILES*NUM_VPS) ; i++ ){
		cheapout admin_sys = (cheapout) &(map->vp_ios[i]);
		cheap admin = cheap_init((void*)(PLATFORM_OCM_MEMORY_LOC+sizeof(cheap_t)), CHEAP_VP_STDOUT_SIZE, sizeof(uint8_t));
		memcpy((void *) &(admin_sys->admin_stdout), admin, sizeof(cheap_t));
		if (verbose) printf("VP stdout: %d\n", i);
	}
	for ( int i =0; i < (NUM_TILES*NUM_VPS) ; i++ ){

		cheapout_user admin_sys = (cheapout_user) &(map->vp_users[i]);
		cheapout_user admin_sys_rem = (cheapout_user) &(remmap->vp_users[i]);
		cheap admin = cheap_init((void *)&(admin_sys_rem->buffer_out), CHEAP_VP_USER_SIZE, sizeof(uint8_t));
		cheap admin2 = cheap_init((void *)&(admin_sys_rem->buffer_in), CHEAP_VP_USER_SIZE, sizeof(uint8_t));
		memcpy((void *) &(admin_sys->out), admin, sizeof(cheap_t));
		memcpy((void *) &(admin_sys->in), admin2, sizeof(cheap_t));
		if (verbose) printf("VP user fifo %d\n", i);
	}

	if (verbose) printf("C-HEAP stdout initialised\n");
	printf("Used %u KiB of memory for fifos in shared memory\n", sizeof(shared_memory_map)/1024);

	munmap((void*)ocm, sizeof(shared_memory_map));
err_close:
	if(memf > 0) close(memf);
err_:
	return err;
}

