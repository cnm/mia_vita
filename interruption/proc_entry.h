#ifndef __PROC_ENTRY_H__
#define __PROC_ENTRY_H__

#define PROC_FILE_NAME "geophone"

extern void create_proc_file(void);
extern void write_to_buffer(unsigned int * value, int64_t timestamp);

extern int read_4samples(uint8_t* be_samples, int64_t* timestamp, uint32_t* offset);

#endif
