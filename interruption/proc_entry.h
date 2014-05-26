#ifndef __PROC_ENTRY_H__
#define __PROC_ENTRY_H__

#define PROC_FILE_NAME "geophone"
/*
 *The idea is to timestamp samples right after the first channel is read.
 */

#define CHANNELS                        4
#define SAMPLE_RATE_HZ                  50
#define SECONDS_IN_BUFFER               5
#define BUFF_SIZE                       (CHANNELS * SAMPLE_RATE_HZ * SECONDS_IN_BUFFER)

/*DATA memory layout:
 *
 *For 3 integers 0xAABBCCDD, DATA is:
 *
 *byte:       0  2  1  0    1  0  2  1    2  1  0  2
 *DATA:       DD CC BB AA | DD CC BB AA | DD CC BB AA
 *Sample:     2-|---1-----|--3---|--2---|----4----|-3
 *be_samples:
 */
typedef struct{
    int64_t timestamp;
    uint32_t data[3];//each sample will hold 4 channels
} sample_t;

extern void create_proc_file(void);
extern void write_to_buffer(unsigned int * value, int64_t timestamp);
extern int read_nsamples(sample_t** be_samples, uint32_t* len_in_samples);

#endif
