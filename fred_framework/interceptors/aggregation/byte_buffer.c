#include <linux/slab.h>

#include "byte_buffer.h"

aggregate_buffer* create_aggregate_buffer(uint32_t len, __be32 ip) {
  aggregate_buffer* b = (aggregate_buffer*) kmalloc(sizeof(aggregate_buffer),
						    GFP_ATOMIC);
  if (!b) {
    printk("%s:%u: Failed to alloc buffer\n", __FILE__, __LINE__);
    return NULL;
  }

  b->start = (char*) kmalloc(len, GFP_ATOMIC);

  if (!b->start) {
    printk("%s:%u: Failed to alloc buffer with size %d B\n", __FILE__,
	   __LINE__, len);
    kfree(b);
    return NULL;
  }

  spin_lock_init(&(b->buffer_lock));
  b->ip = ip;
  b->end = b->head = b->start;
  b->end += len;
  b->head = b->end;
  return b;
}

void free_buffer(aggregate_buffer* b) {
  unlock_buffer(b);
  kfree(b->start);
  kfree(b);
}

char* peek_packet(aggregate_buffer* b) {
  lock_buffer(b);
  if(buffer_data_len(b) == 0){
    unlock_buffer(b);
    return NULL;
  }
  unlock_buffer(b);
  return b->head;
}

int8_t push_bytes(aggregate_buffer* b, char* bytes, int32_t len) {
  lock_buffer(b);
  if (buffer_data_len(b) + len <= buffer_len(b)) {
    b->head -= len;
    memcpy(b->head, bytes, len);
    unlock_buffer(b);
    return 0;
  }
  unlock_buffer(b);
  return -ENOMEM;
}

void reset_buffer(aggregate_buffer* b){
  lock_buffer(b);
  b->head = b->end;
  unlock_buffer(b);
}

int buffer_len(aggregate_buffer* b){
  int i;
  lock_buffer(b);
  i = b->end - b->start;
  unlock_buffer(b);
  return i;
}

int buffer_data_len(aggregate_buffer* b){
  int i;
  lock_buffer(b);
  i = b->end - b->head;
  unlock_buffer(b);
  return i;
}

int cpy_data(char* dst, aggregate_buffer* b){
  int len;

  lock_buffer(b);
  len = b->end - b->head;
  if(len > 0){
    dst = kmalloc(len, GFP_ATOMIC);
    if(!dst){
      printk(KERN_EMERG "Unable to copy buffer to destination.\n");
      dst = NULL;
      unlock_buffer(b);
      return 0;
    }
    
    memcpy(dst, b->head, len);
    unlock_buffer(b);
  
    return len;
  }
  unlock_buffer(b);
  return 0;
}
