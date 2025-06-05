#ifndef __CHEAPOUT_H__
#define __CHEAPOUT_H__

#include <platform.h>
#include <cheap.h>
#include <cheap_s.h>

/**
 * main memory map.
 */
typedef struct cheapout_sys_s
{
	volatile cheap_t admin_stdout;
	volatile char buffer_stdout[CHEAP_BUFFER_SIZE_STDIO];
	// User Fifos
	volatile cheap_t admin_user_out;
	volatile char buffer_user_out[CHEAP_BUFFER_SIZE_SYS];
	volatile cheap_t admin_user_in;
	volatile char buffer_user_in[CHEAP_BUFFER_SIZE_SYS];
} cheapout_sys_t;

typedef cheapout_sys_t* cheapout_sys;


typedef struct cheapout_t {
	volatile cheap_t admin_stdout;
	volatile char buffer_stdout[CHEAP_VP_STDOUT_SIZE];
} cheapout_t;
typedef cheapout_t* cheapout;


typedef struct cheapout_user_t {
	volatile cheap_t out;
	volatile char buffer_out[CHEAP_VP_USER_SIZE];
	volatile cheap_t in;
	volatile char buffer_in[CHEAP_VP_USER_SIZE];
} cheapout_user_t;
typedef cheapout_user_t* cheapout_user;

typedef struct 
{
    cheapout_sys_t vp_kernel[NUM_TILES];
    cheapout_t  vp_ios[NUM_TILES*NUM_VPS];
    cheapout_user_t  vp_users[NUM_TILES*NUM_VPS];
} shared_memory_map;


/**
 * Locations into the global memory map.
 */
static const unsigned int OCM_SIZE = sizeof(cheapout_t);
static const unsigned int OCM_LOC[1]  ={
       	0x40000000,
};

#endif //__CHEAPOUT_H__
