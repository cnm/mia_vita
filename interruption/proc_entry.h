#ifndef __PROC_ENTRY_H__
#define __PROC_ENTRY_H__

#define PROC_FILE_NAME "geophone"

void create_proc_file(void);
void write_to_buffer(unsigned int * value);

#endif
