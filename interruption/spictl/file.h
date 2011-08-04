#ifndef __FILE_H
#define __FILE_H
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <fcntl.h>

// see ~/cvs/app/lib/file.{c,h}
ssize_t readt(int fd, void *buf, size_t count, unsigned *ms);
static inline int fileSetBlocking(int fd,int on) {
  return fcntl(fd, F_SETFL, on ? 0 : (FNDELAY | O_NONBLOCK)) != -1;
}


#endif
