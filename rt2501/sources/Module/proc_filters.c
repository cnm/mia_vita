#ifdef CONFIG_SYNCH_ADHOC

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include "proc_filters.h"
#include "filter_chains.h"

#define PROC_FILE_NAME "synch_filters"

struct proc_dir_entry *proc_file_entry;

typedef struct {
	//indicate if the framework has changed and this buffer should be rebuilt.
	uint8_t dirty;
	uint32_t buffer_size;
	uint32_t buffer_offset;
	char* file_buffer;
} proc_file_contents_cache;

proc_file_contents_cache proc_contents;

/*
 *If the buffer is too short for the contents of filters,
 *this function will realloc it to its double.
 */
static void realloc_proc_contents(void) {
  char* old_buff = proc_contents.file_buffer;
  proc_contents.file_buffer = kmalloc(proc_contents.buffer_size << 1, GFP_ATOMIC);
  if (!proc_contents.file_buffer) {
    printk("%s in %s:%d: kmalloc failed. Unnable to realloc proc entry.\n", __FILE__, __FUNCTION__, __LINE__);
    return;
  }
  proc_contents.buffer_size <<= 1;
  proc_file_entry->size = proc_contents.buffer_size;
  kfree(old_buff);
  memset(proc_contents.file_buffer, 0, proc_contents.buffer_size);
}


static void __append_to_buffer(char* dest, char* orig, uint32_t len, uint32_t dst_offset, uint32_t orig_offset){
  memcpy(dest + dst_offset, orig + orig_offset, len);
}

static void append_to_buffer(char* buff_to_append, uint32_t len) {
  __append_to_buffer(proc_contents.file_buffer, buff_to_append, len, proc_contents.buffer_offset, 0);
  proc_contents.buffer_offset += len;
}

static void fill_proc_contents(void){
  uint32_t i;
  char buff[48] = {0};
  
  proc_contents.buffer_offset = 0;
  memset(proc_contents.file_buffer, 0, proc_contents.buffer_size);
  append_to_buffer(" ID Src Address     Dst Address    SP   DP     \n", 48);

  for(i = 0; i < nfilters; i++){
    memset(buff, ' ', sizeof(buff));
    sprintf(buff, "%d", i);
    buff[strlen(buff)] = ' ';
    if(chains[i]->src_addr != 0){
      sprintf(buff + 4, "%d.%d.%d.%d", NIPQUAD(chains[i]->src_addr));
      buff[strlen(buff)] = ' ';
    }
    if(chains[i]->dst_addr != 0){
      sprintf(buff + 20, "%d.%d.%d.%d", NIPQUAD(chains[i]->dst_addr));
      buff[strlen(buff)] = ' ';
    }
    if(chains[i]->src_port != 0){
      sprintf(buff + 36, "%d", ntohs(chains[i]->src_port));
      buff[strlen(buff)] = ' ';
    }
    if(chains[i]->dst_port != 0){
      sprintf(buff + 42, "%d", ntohs(chains[i]->dst_port));
      buff[strlen(buff)] = ' ';
    }
    if(proc_contents.buffer_offset + sizeof(buff) >= proc_contents.buffer_size)
      realloc_proc_contents();
    append_to_buffer(buff, sizeof(buff));
    if(proc_contents.buffer_offset + 1 >= proc_contents.buffer_size)
      realloc_proc_contents();
    append_to_buffer("\n", 1);
  }
  proc_contents.dirty = 0;
}

static int procfile_read(char *buffer, char **buffer_location, off_t offset, int buffer_length, int *eof, void *data) {
  int how_many_can_we_cpy;

  if (proc_contents.dirty)
    fill_proc_contents();

  how_many_can_we_cpy = (proc_contents.buffer_offset + 1 - offset > buffer_length) ? 
    buffer_length : 
    proc_contents.buffer_offset + 1 - offset;

  if (how_many_can_we_cpy == 0) {
    *eof = 1;
    return 0;
  }

  printk("%s\n", proc_contents.file_buffer);

  memcpy(buffer, proc_contents.file_buffer + offset, how_many_can_we_cpy);
  *buffer_location = buffer;
  return how_many_can_we_cpy;
}

/*Write format:
 *Register filter:
 * R:<filter>
 *Unregister filter:
 * U:id
 */
static int procfile_write(struct file *file, const char *buffer, unsigned long count, void *data) {
  char internal_buffer[2 + sizeof(filter)] = {0};
  filter* f;
  uint8_t i;

  if (copy_from_user(internal_buffer, buffer, count)) {
    return -EFAULT;
  }

  switch(internal_buffer[0]){
  case 'R':
    //Register filter
    if(count - 2 != sizeof(filter)){
      printk("Unable to create filter. User program did not write a filter structure to proc entry.\n");
      return -EINVAL;
    }
    f = kmalloc(sizeof(filter), GFP_ATOMIC);
    if(!f){
      printk(KERN_EMERG "%s:%d: kmalloc failed.\n", __FILE__, __LINE__);
      return 0;
    }

    memcpy(f, internal_buffer + 2, sizeof(filter));
    register_filter(f);
    break;
  case 'U':
    //unregister filter
    memcpy(&i, internal_buffer + 2, sizeof(i));
    if(i >= nfilters){
      printk("Unable to delete filter %d\n", i);
      return -EINVAL;
    }
    unregister_filter(i);
    break;
  }
  return count;
}

#define __DEFAULT_PROC_SIZE__ 1024

static void create_proc_file(void) {
  proc_file_entry = create_proc_entry(PROC_FILE_NAME, 0644, NULL);

  if (proc_file_entry == NULL) {
    printk (KERN_EMERG "Error: Could not initialize /proc/%s\n", PROC_FILE_NAME);
    return;
  }

  proc_file_entry->read_proc = procfile_read;
  proc_file_entry->write_proc = procfile_write;
  proc_file_entry->mode = S_IFREG | S_IRUGO | S_IWUSR | S_IWGRP | S_IWOTH;
  proc_file_entry->uid = 0;
  proc_file_entry->gid = 0;
  proc_file_entry->size = proc_contents.buffer_size;

  memset(&proc_contents, 0, sizeof(proc_file_contents_cache));
  proc_contents.dirty = 1;
  proc_contents.file_buffer = kmalloc(__DEFAULT_PROC_SIZE__, GFP_ATOMIC);
  if (!proc_contents.file_buffer) {
    printk(KERN_EMERG "%s in %s:%d: kmalloc failed. Unnable to create proc entry.\n",  __FILE__, __FUNCTION__, __LINE__);
    return;
  }
  proc_contents.buffer_size = __DEFAULT_PROC_SIZE__;
  memset(proc_contents.file_buffer, 0, __DEFAULT_PROC_SIZE__);
}

/*
 * This is called by the filters to notify this proc entry that the cache needs to be
 * rebuilt.
 * */
void mess_proc_entry(void) {
  proc_contents.dirty = 1;
}

void initialize_proc_filters(void){
  create_proc_file();
}

void teardown_proc_filters(void){
  kfree(proc_contents.file_buffer);
  remove_proc_entry(PROC_FILE_NAME, NULL);
}

#endif
