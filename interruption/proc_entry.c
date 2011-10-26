/*
 * This file creates a read only proc entry which will contain the available interceptors
 * and all the rules registered. Its main purpose is for debugging issues, although it
 * can be used at any time. Why is it for debugging? First, it's an easy way to know
 * if your interceptor is correctly registered and rules are registered. Second, it doen't
 * do anything else besides listing the contents in the framework.
 *
 * Also, keep in mind that proc entries are not regular files and in this case each time
 * a read is issued, the interceptor framework needs to be scanned for registered interceptors
 * and rules. The process is simple, scan the framework, build a text representation of it in
 * memory and export it to user land. However, it can take quite a bit.
 *
 * For this reason, such text representation is cached in a buffer. If multiple reads are
 * issued to this proc entry without any changes to the framework, only the first one should
 * take a huge amount of time. If an interceptor is registered, then when the next read is
 * issued the buffer will be rebuild.
 *
 * If no read is issued the buffer will never be built. Thus in normal execution, this
 * proc entry will not place any kind of overhead.
 * */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/in.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>

#include "proc_entry.h"

#define PROC_FILE_NAME "geophone"

struct proc_dir_entry *proc_file_entry;

unsigned int last_read = 0;
unsigned int last_write = 0;

#define BUFFER_SIZE 12000
unsigned int DATA[BUFFER_SIZE];
unsigned int check_how_many_can_we_copy(unsigned int last_r, unsigned int last_w, unsigned int * over, int buffer_length);

unsigned int check_how_many_can_we_copy(unsigned int last_r, unsigned int last_w, unsigned int * over, int buffer_length){
    unsigned int can_copy = 0;
    *over = 0;

    if(last_r < last_w){
        can_copy = last_w - last_r;

        if(can_copy > buffer_length){
            can_copy = buffer_length;
        }
    }

    else if(last_r > last_w){
        *over = last_w + 1;
        can_copy = (BUFFER_SIZE - last_r);

        if(can_copy + *over > buffer_length){
            if(can_copy > buffer_length){ // The can copy alone is larger than the buffer
                can_copy = buffer_length;
            }

            else { //Only the over is above the buffer size
                *over = buffer_length - can_copy;
            }
        }
    }

    else if(last_r == last_w){
        can_copy = 0;
    }

    else{
        printk("Error: Should never happen");
        return 0;
    }

    return can_copy;
}

//Called by each read to the proc entry. If the cache is dirty it will be rebuilt.
static int procfile_read(char *buffer, char **buffer_location, off_t offset,
                         int buffer_length, int *eof, void *data) {

    unsigned int remaining_buffer = buffer_length;
    unsigned int how_many_we_copy = (remaining_buffer < (BUFFER_SIZE*4) - offset) ? remaining_buffer : (BUFFER_SIZE*4) - offset;

    memcpy(buffer, ((void*) DATA) + offset, how_many_we_copy * 4);

    printk(KERN_EMERG "OFFSET: %d \t, Last read %u \tLast write %u READING: %d \n", (int) offset, last_read % BUFFER_SIZE, last_write % BUFFER_SIZE, how_many_we_copy);

    *eof = 1;
    *buffer_location = buffer;

    return how_many_we_copy;
}

void write_to_buffer(unsigned int * value){
    last_write = ((last_write + 3) % BUFFER_SIZE);

    if (last_write == last_read)
      last_read = last_write - 1;

    DATA[last_write % BUFFER_SIZE] = *value;
    DATA[last_write + 1 % BUFFER_SIZE] = *(value + 1);
    DATA[last_write + 2 % BUFFER_SIZE] = *(value + 1);
}

void create_proc_file(void) {
    proc_file_entry = create_proc_entry(PROC_FILE_NAME, 0644, NULL);

    if (proc_file_entry == NULL) {
        printk (KERN_ALERT "Error: Could not initialize /proc/%s\n",
                PROC_FILE_NAME);
        return;
    }

    proc_file_entry->read_proc = procfile_read;
    proc_file_entry->mode = S_IFREG | S_IRUGO;
    proc_file_entry->uid = 0;
    proc_file_entry->gid = 0;
    proc_file_entry->size = BUFFER_SIZE;
}
