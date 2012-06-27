/*
 * This program is used to set the seconds counter in the modified kernel.
 * The value passed should be read from the GPS device.
 *
 * This program already runs the command to start xuartctl.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>

#include "syscall_wrapper.h"
#include "gps_uartctl.h"
#include "gps_time.h"

/*
 * Most of the times the board creates this device to interact with the GPS.
 * However, I've made this a parameter so you can specify another device in case the gives it another name.
 */
char* gps_device = "/dev/pts/1";

uint8_t parseargs(int argc, char** argv){
  int i;
  for(i = 1; i < argc; i++){
    if(!strcmp(argv[i], "-d")){
      gps_device = argv[i + 1];
      i +=2;
      continue;
    }

    printf("Unknown argument %s\nUsage: %s [-d <gps_device>]\n", argv[i], argv[0]);
    return 0;
  }
  return 1;
}

int main(int argc, char** argv){
  struct timeval tv;

  if(argc > 1 && !parseargs(argc, argv))
    return -1;
    
  printf("Starting xuartctl...");
  fflush(stdout);
  system("xuartctl -d -p 0 -o 8o1 -s 9600");
  printf("done\n");

  printf("Starting gps with device %s\n", gps_device);
  uart_init(1, 1, stderr, gps_device);

  while(!is_gps_ready())
    sleep(1);

  printf("GPS is ready. Reading value and setting it in kernel.\n");
  switch(getGPStimeUTC(&tv)){
  case 1:
    fprintf(stderr, "Unable to read time from GPS (Error 1)\n");
    return -1;
  case 2:
    fprintf(stderr, "An error occured while waiting for a time reply (Error 2)\n");
    return -1;
  }
  set_seconds((uint64_t) tv.tv_sec);
  printf("Program finished without errors.\n");
  return 0;
}
