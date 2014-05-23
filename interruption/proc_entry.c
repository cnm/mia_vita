#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/in.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>

#include "proc_entry.h"
#include "mem_addr.h"

/* This file is responsible to manage the DATA circular buffer.
 * There are two main variables:
 *
 *  last_read = Indicates the ARRAY position where the last read occurred.
 *  Starts at index 0 (indicating that position 0 has been read, so first position to be really read is 1)
 *
 *  last_write = Indicates the ARRAY position where the last write occurred.
 *  Starts at index 0 (indicating that position 0 was written, so the first to be really written is 1)
 *
 *  When a read occurs if last_read == last_write then there is nothing to read
 *  Else it should read as many packets as it is able (until last_write == last_read even if last_read > last_write (remember this is a circular buffer))
 *
 *  If at any time when last_read is incremented it becomes last_write + 1 then the reader is faster than the writer.
 *  In this case the module should crash ungracefully so it is easily noticeablej
 *
 *  If at any time when last_write is incremented it becomes last_read then the writer is much faster than the reader.
 *  In this case the writer should crash.
 *  (altough the state is not yet invalid it is the last valid state. It should not get this close to the invalid state )
 *
 *  Be careful because a write can interrupt a read. The contrary should never happen.
 *  (the write comes from a hardware interruption. The read is a simple kernel thread)
 */

// The struct that represents the proc file entry.
struct proc_dir_entry *proc_file_entry;

// The last write array index
unsigned volatile uint32_t last_write = 0;
unsigned volatile uint32_t last_read = 0;

// The circular buffer
sample DATA[DATA_SIZE];//Note that I've changed DATA_SIZE

#define FIRST_INDEX 0
#define LAST_INDEX DATA_SIZE - 1

/**
 * @brief Ok, reading 1 sample at a time is quite easy. Let's try to read more than one. This function will read as many samples
 *as it can from the DATA circular buffer.
 *
 * @param be_samples - is a pointer which will be allocated inside the function. It will contain the samples in big endian format.
 *        It should be released by the caller of the function.
 *
 * @return 0 nothing is read. 1 if it read correctly something
 */
int read_nsamples(sample** be_samples, uint32_t* len_in_samples)
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
  uint32_t samples_to_copy; // Indicates how many samples to copy (note a sample has the three channels and a timestamp)

  if(*last_read == last_write)
    {
#ifdef __DEBUG__
      printk(KERN_INFO "NOTHING TO READ\n");
#endif
      return 0; // There are no samples to read
    }

  // Let's check if the last_write is behind the last read
  // This can occur as this is a circular buffer
  if(last_read > last_write) {
    // In this case we will read all images until the end of buffer and then from 0 to the last write
    // the + 1  is to read the last write so that last_read == last_write
    samples_to_copy = (LAST_INDEX - last_read) + (last_write - FIRST_INDEX ) + 1:
  }
  // In this case the write is ahead of read
  else  {
    sammples_to_copy = last_write - last_read;
  }

  // Let's create buffer to copy the samples
  *be_samples = kmalloc(samples_to_copy * sizeof(sample), GFP_ATOMIC);

  // Check if the kmalloc return something valid
  if(!(*be_samples))
    {
      printk(KERN_EMERG "%s:%d: Cannot read samples. Kmalloc failed.", __FILE__, __LINE__);
      return 0;
    }

  // for each of the samples copy them (this could be done with a memcopy in future to improve performance)
  for(int i = 1; i <= samples_to_copy; i += 1)
    {
      (*be_samples)[i] = (DATA[(last_read + i) % DATA_SIZE]);
    }

  last_read = (last_read + samples_to_copy) % DATA_SIZE;

  // We have to check if we did everything allright
  last_read == last_write;

  return samples_to_copy; //Read was successful
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
  printk(KERN_INFO "Writint to buffer %d value %u\n", last_write, (*value));
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
