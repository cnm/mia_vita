#ifndef __BYTE_BUFFER_H__
#define __BYTE_BUFFER_H__

#include <linux/kernel.h>
#include <linux/spinlock.h>

/* |------------------------------------|
 * |                  |  2      |   1   |
 * |------------------------------------|
 * ^-start            ^-head            ^-end
 */

typedef struct aggbuff {
  spinlock_t buffer_lock;
  __be32 ip;
  char* start;
  char* end;
  char* head;
} aggregate_buffer;

#define lock_buffer(B)				\
  spin_lock_bh(&((B)->buffer_lock))

#define unlock_buffer(B)			\
  spin_unlock_bh(&((B)->buffer_lock))

extern void reset_buffer(aggregate_buffer* b);
extern int buffer_len(aggregate_buffer* b);
extern int buffer_data_len(aggregate_buffer* b);
extern aggregate_buffer* create_aggregate_buffer(uint32_t len, __be32 ip);
extern void free_buffer(aggregate_buffer* b);
extern char* peek_packet(aggregate_buffer* b);
extern int8_t push_bytes(aggregate_buffer* b, char* bytes, int32_t len);
extern int cpy_data(char* dst, aggregate_buffer* b);
#endif
