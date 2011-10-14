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

int last_read = 0;
int last_write = 0;

#define BUFFER_SIZE 10000
unsigned int buffer[BUFFER_SIZE];

//Called by each read to the proc entry. If the cache is dirty it will be rebuilt.
static int procfile_read(char *buffer, char **buffer_location, off_t offset,
                         int buffer_length, int *eof, void *data) {
    int how_many_can_we_cpy;
    how_many_can_we_cpy = 1;

    memcpy(buffer, last_read + 1, how_many_can_we_cpy);
    *buffer_location = buffer;

    last_read = last_read + how_many_can_we_cpy;

    return how_many_can_we_cpy;
}

void write_to_buffer(unsigned int value){
    last_write = (last_write + 1 % BUFFER_N)

    if (last_write == last_read) /* Just for a simple mark */
      value = 0;

    buffer[last_write % BUFFER_N] = value;
}

static void create_proc_file(void) {
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
