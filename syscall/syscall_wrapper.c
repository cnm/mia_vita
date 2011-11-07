#include <sys/syscall.h>
#include <unistd.h>

#define GPS_SYS_CALL (__NR_SYSCALL_BASE + 356)

long get_millis_offset(){
  long i = syscall(GPS_SYS_CALL);
  if(i == -1){
    perror("%s syscall failed", __FUNCTION__);
    return 0;
  }
  return i;
}
