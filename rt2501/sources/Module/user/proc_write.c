#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "proc_write.h"

//change here if you change this in kernel
typedef struct{
  uint8_t proto;
  uint32_t dst_addr;
  uint32_t src_addr;
  uint16_t dst_port;
  uint16_t src_port;
}filter;

char* proc_entry_name = "/proc/synch_filters";
//-----------------

void write_to_proc_entry(char cmd, uint32_t daddr, uint32_t saddr, uint16_t dport, uint16_t sport, uint8_t index){
  switch(cmd){
  case 'R':{
    char buff[2 + sizeof(filter)] = {0};
    int fd, status;
    filter f;
    buff[0] = 'R';
    buff[2] = ':';

    f.proto = 17; //UDP
    f.dst_addr = daddr;
    f.src_addr = saddr;
    f.dst_port = dport;
    f.src_port = sport;

    memcpy(buff + 2, &f, sizeof(filter));
    
    fd = open(proc_entry_name, O_WRONLY);
    if(fd == -1){
      perror("open()");
      return;
    }
    
    status = write(fd, buff, sizeof(buff));
    if(status == -1){
      perror("write()");
      close(fd);
      return;
    }
    close(fd);
    break;
  }
  case 'U':{
    char buff[3] = {0};
    int fd, status;
    buff[0] = 'U';
    buff[2] = ':';

    memcpy(buff + 2, &index, sizeof(index));
    
    fd = open(proc_entry_name, O_WRONLY);
    if(fd == -1){
      perror("open()");
      return;
    }
    
    status = write(fd, buff, sizeof(buff));
    if(status == -1){
      perror("write()");
      close(fd);
      return;
    }
    close(fd);
    break;
  }
  default:
    fprintf(stderr, "Unknown command %c\n", cmd);
    return;
  }
  return;
}
