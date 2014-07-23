/*
 * uart_gps_test.c
 *
 *  Created on: Jun 13, 2011
 *      Author: ricardo
 */

#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#include "gps_time.h"
#include "gps_uartctl.h"

#define GPS_DEVICE "/dev/pts/0"

/* Subtract the `struct timeval' values X and Y,
   storing the result in RESULT.
   Return 1 if the difference is negative, otherwise 0.  */


int main(int argc, char **argv) {
    unsigned int i;
    for(i = 1; i < argc;){
        if(!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")){
            printf("This is the help function");
            return 0;
        }
    }


    int x;
    struct tm *broken_time;
    struct timeval t;
    double lat, lon, alt;

    uart_init(0, stderr, GPS_DEVICE);

    while (!is_gps_ready()) {
        sleep(1);
    }

    printf("GPS is ready\n");

    for (x = 0; x < 1; x++) {
    	// read time
        getGPStimeUTC(&t);
        broken_time = localtime(&(t.tv_sec));

        //printf("%ld sec %ld usec\n", t.tv_sec, t.tv_usec);
        printf("Current date: %02d/%02d/%02d %02d:%02d:%02d.%06d\n",
               broken_time->tm_mday, broken_time->tm_mon,
               broken_time->tm_year + 1900, broken_time->tm_hour,
               broken_time->tm_min, broken_time->tm_sec, (int) t.tv_usec);

        // read position
        getGPSLLA( &lat, &lon, &alt);
        printf("latitude %f\n", lat);
        printf("longitude %f\n", lon);
        printf("altitude %f\n", alt);
    }

    return 0;
}
