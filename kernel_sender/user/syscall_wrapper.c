#include <sys/syscall.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <stdio.h>


#include "syscall_wrapper.h"

//for arm
#ifdef __NR_SYSCALL_BASE
  #define GPS_SET_SECONDS_SYS_CALL (__NR_SYSCALL_BASE + 354)
  #define GPS_MEAN_SYS_CALL (__NR_SYSCALL_BASE + 355)
  #define GPS_SYS_CALL (__NR_SYSCALL_BASE + 356)
#else //just to compile for x86
  #define GPS_SET_SECONDS_SYS_CALL (354)
  #define GPS_MEAN_SYS_CALL (355)
  #define GPS_SYS_CALL (356)
#endif

uint64_t get_millis_offset(){
  uint64_t res;
  int i = syscall(GPS_SYS_CALL, &res);
  if(i == -1){
    perror("get_millis_offset syscall failed");
    exit(-1);
  }

  return res;
}

uint64_t get_mean_value(){
  uint64_t res;
  int i = syscall(GPS_MEAN_SYS_CALL, &res);
  if(i == -1){
  	perror("get_mean_value syscall failed");
  	exit(-1);
  }
  return res;
}

void set_seconds(uint64_t s){
  int i = syscall(GPS_SET_SECONDS_SYS_CALL, &s);
  if(i == -1){
    perror("setseconds syscall failed");
    exit(-1);
  }
}
