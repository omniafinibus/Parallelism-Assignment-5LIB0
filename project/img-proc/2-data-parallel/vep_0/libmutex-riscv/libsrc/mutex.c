#include <stdint.h>
#include <xil_printf.h>
#include <stdlib.h>
#include <platform.h>
#include <mutex.h>
#ifndef ARM
#define printf xil_printf
#include <xil_printf.h>
#endif

/* Kees Goossens
 * 2022-03-18 v1
 * 2022-06-15 allow mtx_admin access only via ring to avoid possibility of simultaneous read
 */

/* assumptions: sequential consistency model & atomic registers */

#define DOWN 0
#define UP 1

#ifndef ARM

mutex_t * mutex_init(mutex_t volatile * const mtx_admin)
{
  if (mtx_admin == NULL) return NULL;
  mtx_admin->flag[0] = DOWN;
  mtx_admin->flag[1] = DOWN;
  mtx_admin->after_you = 0;
  return mtx_admin;
}

void mutex_destroy(mutex_t volatile * const mtx_admin)
{
  mutex_init(mtx_admin);
}

#endif

bool mutex_initialized(mutex_t volatile const * const mtx_admin)
{
  return (mtx_admin != NULL ? true : false);
}

void mutex_print_status(mutex_t volatile const * const mtx_admin)
{
  if (mtx_admin == NULL)
    printf("mutex=NULL\n");
  else
    printf("mutex=0x%08p flag={%d,%d} after_you=%d\n",
           mtx_admin, mtx_admin->flag[0], mtx_admin->flag[1], mtx_admin->after_you);
}

void mutex_claim(mutex_t volatile * const mtx_admin, uint32_t const me)
{
  if (mtx_admin == NULL || me > 1) {
    printf("mutex_claim: error: invalid args mtx_admin=%p me=%d\n",me);
    exit (EXIT_FAILURE);
  }
  mtx_admin->flag[me] = UP;
  mtx_admin->after_you = me;
  while (mtx_admin->flag[1-me] == UP && mtx_admin->after_you == me) ; // spin lock
}

void mutex_release(mutex_t volatile * const mtx_admin, uint32_t const me)
{
  if (mtx_admin == NULL || me > 1) {
    printf("mutex_release: error: invalid args mtx_admin=%p me=%d\n",me);
    exit (EXIT_FAILURE);
  }
  mtx_admin->flag[me] = DOWN;
}
