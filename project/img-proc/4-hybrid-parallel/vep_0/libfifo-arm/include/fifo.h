#ifndef _FIFOH_
#define _FIFOH_

#include <stdint.h>
#include <stdbool.h>

typedef volatile struct _fifo_t {
  uint8_t dummy[36];
} fifo_t;

// token_buffer must point to an array containing nr_tokens that are aligned
// function returns NULL on failure and fifo_admin on success
extern fifo_t * fifo_init(fifo_t volatile * const fifo_admin, void volatile * const token_buffer, uint32_t const nr_tokens, uint32_t const token_size);
// if token_buffer was malloc'd before fifo_init then it must be freed after fifo_destroy
extern void fifo_destroy(fifo_t volatile * const fifo_admin);
// fifo_initialized must return false before fifo_init has been called successfully
// or after fifo_destroy has been called, and true otherwise
extern bool fifo_initialized(fifo_t volatile const * const fifo_admin);
extern void fifo_print_status(fifo_t volatile const * const fifo_admin);
// use fifo_spaces/tokens to poll, for non-blocking behaviour
// fifo_spaces/tokens returns the number of spaces/tokens that can still be claimed
extern uint32_t fifo_spaces(fifo_t volatile const * const fifo_admin);
extern uint32_t fifo_tokens(fifo_t volatile const * const fifo_admin);
// all following functions are blocking
#ifdef ARM
// return the index of the space/token in the token array
extern uint32_t fifo_claim_space(fifo_t volatile * const fifo_admin);
extern uint32_t fifo_claim_token(fifo_t volatile * const fifo_admin);
#else
// return a pointer to the space/token
extern void volatile * fifo_claim_space(fifo_t volatile * const fifo_admin);
extern void volatile * fifo_claim_token(fifo_t volatile * const fifo_admin);
// write/read = claim + release
// can only write/read when no spaces/tokens have been claimed, to avoid confusion
extern void fifo_write_token(fifo_t volatile * const fifo_admin, void const * const token_to_write);
extern void fifo_read_token(fifo_t volatile * const fifo_admin, void * const new_token);
#endif
extern void fifo_release_token(fifo_t volatile * const fifo_admin);
extern void fifo_release_tokens(fifo_t volatile * const fifo_admin, uint32_t const tokens);
extern void fifo_release_space(fifo_t volatile * const fifo_admin);
extern void fifo_release_spaces(fifo_t volatile * const fifo_admin, uint32_t const spaces);

#endif

