#ifndef _mutexH_
#define _mutexH_

/* Kees Goossens
 * 2022-03-18 v1
 */

#include <stdbool.h>

typedef volatile struct {
  uint32_t flag[2];   // I'd like to enter, initially DOWN
  uint32_t after_you; // me indicating that you can go first, any initial value
} mutex_t;

#ifndef ARM
// mutex_init assumes that mtx_admin points to space for (mutex_t *) malloc (sizeof(mutex));
// function returns NULL on failure and mtx_admin on success
// RISC-V must initialise the mutex
extern mutex_t * mutex_init(mutex_t volatile * const mtx_admin);
extern void mutex_destroy(mutex_t volatile * const mtx_admin);
#endif
extern bool mutex_initialized(mutex_t volatile const * const mtx_admin);
extern void mutex_print_status(mutex_t volatile const * const mtx_admin);
extern void mutex_claim(mutex_t volatile * const mtx_admin, uint32_t const me);
extern void mutex_release(mutex_t volatile * const mtx_admin, uint32_t const me);

#endif

