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
#include "mem_addr.h"


struct proc_dir_entry *proc_file_entry;

unsigned int last_write = 0;

unsigned int DATA[DATA_SIZE];



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
        can_copy = (DATA_SIZE - last_r);

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

/*
 *offset is an in/out parameter expressed in 32bit word size. For example, an offset of 3 means we have to do DATA+3.
 *be_samples should be 12 bytes long.
 */
int read_4samples(uint8_t* be_samples, uint32_t* offset){
  /*DATA memory layout:
   *
   *For 3 integers 0xAABBCCDD, DATA is:
   *
   *byte:       0  2  1  0    1  0  2  1    2  1  0  2
   *DATA:       DD CC BB AA | DD CC BB AA | DD CC BB AA
   *Sample:     2-|---1-----|--3---|--2---|----4----|-3
   *be_samples:
   */

  uint8_t* int1 = (uint8_t*) (DATA + (*offset % DATA_SIZE));
  uint8_t* int2 = (uint8_t*) (DATA + ((*offset + 1) % DATA_SIZE));
  uint8_t* int3 = (uint8_t*) (DATA + ((*offset + 2) % DATA_SIZE));

  uint32_t i;
  printk("%s: Offset at %u, last_write %u\n", __FUNCTION__, *offset, last_write);
  for(i = 0; i < 24; i++){
    printk("%02X ", int1[i]);
    if(i != 0 && i % 8 == 0)
      printk("\n");
  }
  printk("\n");

  if(*offset % DATA_SIZE == last_write)
    return 0; //Screw this... cannot read samples

  be_samples[0] = int1[3];
  be_samples[1] = int1[2];
  be_samples[2] = int1[1];

  be_samples[3] = int1[0];
  be_samples[4] = int2[2];
  be_samples[5] = int2[3];

  be_samples[6] = int2[1];
  be_samples[7] = int2[0];
  be_samples[8] = int3[3];

  be_samples[9] = int3[2];
  be_samples[10] = int3[1];
  be_samples[11] = int3[0];

  *offset = (*offset + 3) % DATA_SIZE;
  return 1; //Read was successful 
}
EXPORT_SYMBOL(read_4samples);

//Called by each read to the proc entry. If the cache is dirty it will be rebuilt.
static int procfile_read(char *dest_buffer, char **buffer_location, off_t offset,
                         int dest_buffer_length, int *eof, void *data) {

    /* We only use char sizes from here */
    unsigned int data_size_in_chars = DATA_SIZE*sizeof(char);
    unsigned int last_read = offset % (data_size_in_chars);
    unsigned int how_many_we_copy;

    if (last_read <= last_write){
        how_many_we_copy = last_write - last_read;

        printk(KERN_EMERG "1 Last read %u \tLast write %u READING: %d \n", last_read, last_write % DATA_SIZE, how_many_we_copy);

    }

    else{ /* if (last_read > last_write){ */
        how_many_we_copy = data_size_in_chars - last_read;
        printk(KERN_EMERG "2 Last read %u \tLast write %u READING: %d \n", last_read, last_write % DATA_SIZE, how_many_we_copy);
    }

    memcpy(dest_buffer, ((char*) DATA) + last_read, how_many_we_copy);

    *eof = 0;
    *buffer_location = dest_buffer;

    return how_many_we_copy;
}

/* This function is called by the interruption and therefore cannot be interrupted */
void write_to_buffer(unsigned int * value){
/*    printk(KERN_INFO "Writint to buffer %d value %u\n", last_write, (*value));*/

  
    /* FRED CHANGE THIS */  
  /*    *value = 0x11223344;
    *(value + 1) = 0x55667788;
    *(value + 2) = 0x99AABBCC;*/

    DATA[last_write] = *value;
    DATA[(last_write + 1) % DATA_SIZE] = *(value + 1);
    DATA[(last_write + 2) % DATA_SIZE] = *(value + 2);

    last_write = ((last_write + 3) % DATA_SIZE);
}

void create_proc_file(void) {
    last_write = 0;
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
    proc_file_entry->size = DATA_SIZE;
}
