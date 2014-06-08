#ifndef __PROC_ENTRY_H__
#define __PROC_ENTRY_H__

#define PROC_FILE_NAME "geophone"
/*
 *The idea is to timestamp samples right after the first channel is read.
 */

#define CHANNELS                        4
#define SAMPLE_RATE_HZ                  50
#define SECONDS_IN_BUFFER               5


/* The size of the transmit circular buffer.  This must be a power of two. It should be close to (CHANNELS * SAMPLE_RATE_HZ * SECONDS_IN_BUFFER) */
#define CIRC_BUF_SIZE                        1024

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
    int32_t seq_number;
    uint32_t data[3];//each sample will hold 4 channels
} sample_t;

/**
 * @brief This presents a fast way of doing a module (%) of a power of two buffer size
 *
 * Let BUFFER_SIZE be the buffer size
 *
 * First we create a mask with:
 *     (BUFFER_SIZE - 1)
 *
 *  Now we do the and (&) operation with the index
 *
 *     index & (BUFFER_SIZE - 1)
 *
 *  And only the bits "lower" than buff remain. This is genious.
 *
 * Reference:
 *    http://lxr.free-electrons.com/source/include/linux/circ_buf.h#L25
 *    https://android.googlesource.com/toolchain/benchmark/+/honeycomb/android_build/eclair/bionic/libc/kernel/common/linux/circ_buf.h
 *
 * @param index The index to be moduled
 *
 * @return The results of (index % BUFFER_SIZE)
 */
static inline unsigned int quick_module(unsigned int index) {
    return index & (CIRC_BUF_SIZE - 1);
}


extern void create_proc_file(void);
extern void write_to_buffer(unsigned int * value, int64_t timestamp, int32_t seq_number);
extern uint32_t read_nsamples(sample_t** be_samples);

#endif
