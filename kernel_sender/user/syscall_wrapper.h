#ifndef __SYSCALL_WRAPPER_H__
#define __SYSCALL_WRAPPER_H__

#include <stdint.h>

extern uint64_t get_millis_offset();
extern uint64_t get_mean_value();
extern void set_seconds(uint64_t s);
#endif
