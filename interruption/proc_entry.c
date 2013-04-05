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

sample DATA[DATA_SIZE];//Note that I've changed DATA_SIZE

/*
 *Ok, reading 1 sample at a time is quite easy. Let's try to read more than one. This function will read as many samples
 *as it can.
 *
 *be_samples is a pointer which will be allocated inside the function. It will contain the samples in big endian format.
 *len is a pointer to the number of copied samples (one sample has the three channels).
 */
int read_nsamples(sample** be_samples, uint32_t* len_in_samples, uint32_t* last_read)
{
    /*DATA memory layout:
     *
     *For 3 integers 0xAABBCCDD, DATA is:
     *
     *byte:       0  2  1  0    1  0  2  1    2  1  0  2
     *DATA:       DD CC BB AA | DD CC BB AA | DD CC BB AA
     *Sample:     2-|---1-----|--3---|--2---|----4----|-3
     *be_samples:
     */
    uint32_t samples_to_copy, i;

    if(*last_read == last_write)
      {
#ifdef __DEBUG__
      printk(KERN_INFO "NOTHING TO READ\n");
#endif
      return 0; //Screw this... cannot read samples
      }

    samples_to_copy = (*last_read > last_write)? last_write + DATA_SIZE - *last_read : last_write - *last_read;

    *be_samples = kmalloc(samples_to_copy * sizeof(sample), GFP_ATOMIC);

    if(!(*be_samples))
      {
        printk(KERN_EMERG "%s:%d: Cannot read samples. Kmalloc failed.", __FILE__, __LINE__);
        return 0;
      }

    for(i = 0; i < samples_to_copy; i += 1)
      {
        (*be_samples)[i] = (DATA[(*last_read + i) % DATA_SIZE]);
      }

    *last_read = (*last_read + samples_to_copy) % DATA_SIZE;
    *len_in_samples = samples_to_copy;
    return 1; //Read was successful
}
EXPORT_SYMBOL(read_nsamples);

//Called by each read to the proc entry. If the cache is dirty it will be rebuilt.
static int procfile_read(char *dest_buffer, char **buffer_location, off_t offset,
                         int dest_buffer_length, int *eof, void *data)
  {

    /* We only use char sizes from here */
    unsigned int data_size_in_chars = DATA_SIZE * sizeof(sample);
    unsigned int how_many_we_copy = 0;

    //Calculates how many octets to copy
    if (offset <= data_size_in_chars)
      { //If offset asked is inferior to the size array
        how_many_we_copy = data_size_in_chars - offset;
#ifdef __DEBUG__
        printk(KERN_EMERG "Last read %u \tLast write %u READING: %d \n", (int) offset, data_size_in_chars, how_many_we_copy);
#endif
      }
    else
      {
        how_many_we_copy = 0;
      }

    how_many_we_copy = (how_many_we_copy < dest_buffer_length) ? how_many_we_copy : dest_buffer_length;

    memcpy(dest_buffer, ((char*) DATA) + offset, how_many_we_copy);

    *eof = how_many_we_copy ? 1 : 0;
    *buffer_location = dest_buffer;

    return how_many_we_copy;
  }

/* This function is called by the interruption and therefore cannot be interrupted */
void write_to_buffer(unsigned int * value, int64_t timestamp)
{
#ifdef __DEBUG__
/*    printk(KERN_INFO "Writint to buffer %d value %u\n", last_write, (*value));*/
#endif

    DATA[last_write].timestamp = timestamp;
    DATA[last_write].data[0] = *value;
    DATA[last_write].data[1] = *(value + 1);
    DATA[last_write].data[2] = *(value + 2);

    last_write = ((last_write + 1) % DATA_SIZE);
}

void create_proc_file(void)
{
    last_write = 0;
    proc_file_entry = create_proc_entry(PROC_FILE_NAME, 0644, NULL);

    if (proc_file_entry == NULL)
      {
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
