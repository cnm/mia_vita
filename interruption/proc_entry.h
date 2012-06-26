#ifndef __PROC_ENTRY_H__
#define __PROC_ENTRY_H__

#define PROC_FILE_NAME "geophone"
/*
 *The idea is to timestamp samples right after the first channel is read.
 */
typedef struct{

#ifdef __GPS__
    int64_t gps_us;
#endif

    int64_t timestamp;
    uint32_t data[3];//each sample will hold 4 channels
}sample;


extern void create_proc_file(void);

#ifdef __GPS__
extern void write_to_buffer(unsigned int * value, int64_t timestamp, int64_t gps_us);
extern int read_nsamples(sample** be_samples, uint32_t* len_in_samples, int64_t* gps_us, uint32_t* last_read);
#else
extern void write_to_buffer(unsigned int * value, int64_t timestamp);
extern int read_nsamples(sample** be_samples, uint32_t* len_in_samples, uint32_t* last_read);
#endif

#endif
