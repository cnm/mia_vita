/*
 * This program is used to set the seconds counter in the modified kernel.
 * The value passed should be read from the GPS device.
 *
 * It reads the current time and sets the the seconds variable in the MIAVITA KERNEL
 *          __miavita_elapsed_secs
 *
 * This program already runs the command to start xuartctl.
 *
 * It receives two parameters:
 *      -d indicates the gps device (default is /dev/pts/0)
 *      -t number of tries for the GPS to be ready
 *
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
char* gps_device = "/dev/pts/0";
unsigned int tries = 15;
unsigned int original_tries;

uint8_t parseargs(int argc, char** argv){
    int i;
    for(i = 1; i < argc; i++){
        if(!strcmp(argv[i], "-d")){
            gps_device = argv[i + 1];
            i += 2;
            continue;
        }

        if(!strcmp(argv[i], "-t")){
            tries = atoi(argv[i + 1]);
            i += 2;
            continue;
        }

        printf("Unknown argument %s\nUsage: %s [-d <gps_device>] -t <number_tries>\n", argv[i], argv[0]);
        return 0;
    }
    return 1;
}

static void print_time(){
    time_t rawtime;
    struct tm * timeinfo;

    time (&rawtime);
    timeinfo = localtime (&rawtime);
    printf("Current local time and date: %s\n", asctime (timeinfo) );
}

int main(int argc, char** argv){
    struct timeval tv;

    if(argc > 4 || parseargs(argc, argv) == 0)
      return -1;

    /* This starts the xuartctl daemon TODO - Should we start it */
    /*  printf("Starting xuartctl...");*/
    /*  fflush(stdout);*/
    /*  system("xuartctl -d -p 0 -o 8n1 -s 9600");*/
    /*  printf("done\n");*/

    printf("Starting gps with device %s\n", gps_device);
    uart_init( 0, stderr, gps_device);

    original_tries = tries;
    for(;!is_gps_ready(); tries--)
      {
        if(tries <= 0)
          {
            fprintf(stderr, "Number of tries limit of %u was reached. Returning error -1\n", original_tries);
            return -1;
          }
        sleep(1);
      }

    printf("GPS is ready. Reading value and setting it in kernel.\n");
    switch(getGPStimeUTC(&tv)){
      case 1:
        fprintf(stderr, "Unable to read time from GPS (Error 1)\n");
        return -1;
      case 2:
        fprintf(stderr, "An error occured while waiting for a time reply (Error 2)\n");
        return -1;
    }


    printf("GPS returned with UNIX seconds: %ld\n", tv.tv_sec);
    // set miavita time
    set_seconds((uint64_t) tv.tv_sec); /* Set's the seconds in the miavia counter in the kernel cat ts7500_kernel/ipc/miavita_syscall.c:sys_miavitasetseconds */
    printf("Seconds set in the miavita kernel variable: %lu\n", get_mean_value());

    printf("Now I'm going to set the current date to the OS\n");
    printf("Time at before setting:\n");
    print_time();

    // set system time
    time_t seconds_time = (time_t) tv.tv_sec;
    stime(&seconds_time);

    printf("Time at after setting:\n");
    print_time();

    printf("Program finished without errors.\n");
    return 0;
}
