#ifndef __PROC_WRITE_H__
#define __PROC_WRITE_H__

#include <stdint.h>

extern void write_to_proc_entry(char cmd, uint32_t daddr, uint32_t saddr, uint16_t dport, uint16_t sport, uint8_t index);

#endif
