#include "file.h"
#include "mytime.h"

/*
  readt
    read bytes to <buf>
    returns when bytes read reaches <count>
    or when <ms> milliseconds have elapsed, whichever comes first
    returns the number of bytes read
  assumptions: file is open in non-blocking mode

  TODO: 
  * if <ms> is a null pointer, function will wait indefinitely
  * if count is zero, wait <ms> or until data is ready, then return
*/
ssize_t readt(int fd, void *buf, size_t count, unsigned *ms) {
  fd_set readset;
  ssize_t rc,bytesRead = 0;
  struct timeval tv;
  Time tStart,tNow,tTot = 1000LL * *ms;

  tStart = tNow = TimeNow();
  //fprintf(stderr,"start=%lld\n",tStart);
  while (bytesRead < count && tTot - tNow + tStart > 0) {
    //fprintf(stderr,"bytesRead=%d < count=%d, t=%lld > 0\n",bytesRead,count,tTot - tNow + tStart);
    FD_ZERO(&readset);
    FD_SET(fd,&readset);

    tv = TimeToTV(tTot - tNow + tStart);
    //fprintf(stderr,"waiting %d;%d\n",tv.tv_sec,tv.tv_usec);
    if (select(fd+1,&readset,0,0,&tv) > 0) {
      rc = read(fd,buf+bytesRead,count-bytesRead);
      //fprintf(stderr,"read=%d\n",rc);
      if (rc > 0) {
	bytesRead += rc;
      } else {
	break;
      }
    }
    tNow = TimeNow();
  }
  *ms = (tTot - tNow + tStart)/1000;
  //fprintf(stderr,"tTot=%lld, tNow=%lld, tStart=%lld\n",tTot,tNow,tStart);
  return bytesRead;
}

// TO DO: writet() that checks the return of write() and waits to send data
