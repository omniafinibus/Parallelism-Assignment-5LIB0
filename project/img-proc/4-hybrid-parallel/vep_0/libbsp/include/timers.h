#ifndef TIMERS_H
#define TIMERS_H

#include <stdint.h>
#include <platform.h>

// NOTE: wall/global time is uptime since system boot

// use a .h file to allow the use of inline function, for speed & accuracy of reading timers
// read the 64-bit global or partition timer, ensuring consistent 32-bit words
// WARNING: this does NOT work with -Os compile option (which probably turns off inline)


#define read_wall_timer _read_global_timer
#define read_global_timer _read_global_timer
inline uint64_t _read_global_timer()
{
  uint64_t volatile const * const timer = (uint64_t *) GLOBAL_TIMER;
  uint32_t volatile const * const timer_ls = (uint32_t *) GLOBAL_TIMER;
  uint64_t t1;
  uint32_t t2_ls;
  while (1) {
    t1 = *timer;
    t2_ls = *timer_ls;
    if (t2_ls >= ((uint32_t) t1) + 12 && t2_ls <= ((uint32_t) t1) + 18) return t1;
  }
}

// is the resolution of float enough? double is much slower
#define read_wall_timer_sec _read_global_timer_sec
#define read_global_timer_sec _read_global_timer_sec
inline float _read_global_timer_sec()
{
  return ((1.0*_read_global_timer())/FREQUENCY);
}

#define wait_for_wall_time _wait_for_global_time
#define wait_for_global_time _wait_for_global_time
inline float _wait_for_global_time(uint64_t const duration)
{
  uint64_t start = _read_global_timer();
  uint64_t const stop = start + duration;
  while (_read_global_timer() < stop) ;
}

// note: wall time since system boot
#define wait_until_wall_time _wait_until_global_time
#define wait_until_global_time _wait_until_global_time
inline float _wait_until_global_time(uint64_t const stop)
{
  while (_read_global_timer() < stop) ;
}

#define read_virtual_timer _read_partition_timer
#define read_partition_timer _read_partition_timer
inline uint64_t _read_partition_timer()
{
  uint64_t volatile const * const timer = (uint64_t *) PARTITION_TIMER;
  uint32_t volatile const * const timer_ls = (uint32_t *) PARTITION_TIMER;
  uint64_t t1;
  uint32_t t2_ls;
  while (1) {
    t1 = *timer;
    t2_ls = *timer_ls;
    if (t2_ls >= ((uint32_t) t1) + 12 && t2_ls <= ((uint32_t) t1) + 18) return t1;
  }
}

#define read_virtual_timer_sec _read_partition_timer_sec
#define read_partition_timer_sec _read_partition_timer_sec
inline float _read_partition_timer_sec()
{
  return ((1.0*_read_partition_timer())/FREQUENCY);
}

#define wait_for_virtual_time _wait_for_partition_time
#define wait_for_partition_time _wait_for_partition_time
inline float _wait_for_partition_time(uint64_t const duration)
{
  uint64_t start = _read_partition_timer();
  uint64_t const stop = start + duration;
  while (_read_partition_timer() < stop) ;
}

// waiting for virtualised seconds doesn't really make sense

#endif
