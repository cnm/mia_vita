#ifndef __MYTIME_H
#define __MYTIME_H
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
typedef long long Time; // number of microseconds

static inline Time TimeNow() {
  struct timeval tv;

  gettimeofday(&tv,0);
  return ((long long)tv.tv_sec)*1000000 + tv.tv_usec;
}

static inline struct timeval TimeToTV(Time t) {
  struct timeval tv;

  tv.tv_sec = t / 1000000LL;
  tv.tv_usec = t % 1000000LL;
  return tv;
}

#endif
