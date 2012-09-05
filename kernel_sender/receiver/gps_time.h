/*
 * gps_time.h
 *
 *  Created on: Jun 13, 2011
 *      Author: ricardo
 */

#ifndef GPS_TIME_H_
#define GPS_TIME_H_

#define VERBOSE 1

typedef enum {
	GPS_OK,
	GPS_ANTENA_SHORT,
	GPS_ANTENA_OPEN,
	GPS_NEEDED_RESET,
	GPS_BATTERY_FAULT
} gps_fault_t;


// Used by serial port interface
void uart_init( char indoor, char reset, FILE *status_output, char * gps_device);

void output_from_gps( unsigned char* msg, int msg_len);



// available for user
char is_gps_ready();

int getGPStimeUTC( struct timeval *tv);

int getGPSLLA(float *latitude, float * longitude, float *altitude);

int getgpssatellites();

gps_fault_t getgpsfault();


#endif /* GPS_TIME_H_ */
