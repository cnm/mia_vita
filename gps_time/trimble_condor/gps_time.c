/*
 * gps_uartctl.c
 *
 *  Created on: Jun 13, 2011
 *      Author: ricardo
 */

#include <sys/time.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <math.h>
#include <string.h>
#include <time.h>

#include "gps_time.h"

#define GPS_START_CHAR '$'
#define GPS_END_CHAR '\n'
#define GPS_CMD_RESET "$PMTK101*32\r\n" // Hot Restart: Use all available data in the NV store.
// if you change this, recalculate checksum using http://www.hhhh.org/wiml/proj/nmeaxor.html
#define GPS_CMD_SETUP "$PMTK314,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0*28\r\n$PMTK300,1000,0,0,0,0*1C\r\n$PMTK324,0,0,1,0,69*15\r\n"  //  Define automatic GGA and ZDA messages. Set the rate of position fixing activity to 1000ms. Enable PPS always with pulse width 69*61ns.

#define GPS_MSG_STATUS_TIME_POS "GPGGA"
#define GPS_MSG_ANTENNA "PMTKANT"
#define GPS_MSG_TIME "GPZDA"


gps_fault_t gps_fault = GPS_OK;

int num_satellites = 0;

/* enables debug messages */
//#define GPS_DEBUG 1

#define MSG_BUF_SIZE 1024
#define INST_BUF_SIZE 100

/** circular buffer for storing messages from the GPS for processing */
char msg_buf[MSG_BUF_SIZE];
/** buffer for storing single message/instruction. Is required as strcmp does not work on circular buffer */
char inst_buf[INST_BUF_SIZE];
/** first (inclusive) and last (exclusive) positions of data in the circular buffer */
int buff_start = 0, buff_end = 0;

/* Lock for acessing GPS state and circular buffer */
pthread_mutex_t state_change_m = PTHREAD_MUTEX_INITIALIZER;

/* notify message handling thread that new data is available */
pthread_cond_t data_available_cond = PTHREAD_COND_INITIALIZER;
/* notify time request handling thread that time reply is available */
pthread_cond_t reply_available_cond = PTHREAD_COND_INITIALIZER;

// structure to store time reply
struct timeval *current_time = NULL;

// variables for storing current position
double current_latitude = 0;
double current_longitude = 0;
double current_altitude = 0;

FILE *debug_f;

pthread_t process_msg_t;

typedef enum {
	GPS_PRE_INIT, // before GPS reset, dump all messages
	GPS_RESET, // after GPS reset, before first post GPS message is received
	GPS_SEARCH, // GPS is reset. Waiting for GPS signal. Yet unable to provide time
	GPS_READY, // has gps signal. Can provide time
	GPS_QUERY
// waiting from reply to time/position request
// sent a query request. Waiting for reply
} gps_state_t;

gps_state_t gps_state = GPS_PRE_INIT;

void (*gps_write)(char *msg, int msg_len);
void* process_msg(void *unused);


void packet_content(char *error_msg, int start, int end) {
	int x;

	fprintf(debug_f, "Packet content %d to %d. %s: ", start, end, error_msg);

	for (x = start; x != end; x++)
		fprintf(debug_f, "%c", msg_buf[x % MSG_BUF_SIZE]);

	fprintf(debug_f, "\n");

}

/* searches the buffer for a new message
 * receives the start (inclusive) and end (exclusive) position of data in the buffer
 * If a message is found, start and end will define its first and last position
 * message will be unescaped and shifted towards its end, start will point to its first position
 * and end to its last (exclusive)
 * If a message is not found, start and end will be undefined
 * returns 1 if a message is found, 0 otherwise
 */
char lookup_packet(int *start, int *end) {
	int pos;
	int x;

	if( *end == *start) {
		return 0;
	}

	// "make" circular buffer linear
	if (*end < *start)
		*end += MSG_BUF_SIZE;

	// do we have enough data for a message?
	if (*end - *start < 8)
		return 0;

	// does buffer start with a message?
	if (msg_buf[*start] != GPS_START_CHAR) {
		packet_content(
				"Message does not start with $. Should never happen rewrite code!",
				*start, *end);
		exit(1);
	}

	// look for message end
	for (pos = *start; pos < *end && msg_buf[pos%MSG_BUF_SIZE] != GPS_END_CHAR; pos++);

	if (msg_buf[pos%MSG_BUF_SIZE] != GPS_END_CHAR || pos == *end)
		return 0;

	// a packet was found. Let's copy it to the instruction buffer for processing, removing the header $ and trailler \r\n
	for( x = 0; x < pos-*start - 2; x++)
		inst_buf[x] = msg_buf[(*start + 1 + x)%MSG_BUF_SIZE];
	inst_buf[x] = '\0';

	*end = (++pos)%MSG_BUF_SIZE;
	return 1;
}

void init_gps( char reset,
		void (*input_for_gps)(char *msg, int msg_len), FILE *output_status_func) {
	debug_f = output_status_func;

	gps_write = input_for_gps;

	if (reset) {
		// reset gps device
		gps_write(GPS_CMD_RESET, sizeof(GPS_CMD_RESET));
		gps_state = GPS_RESET;
		// throw away messages while resetting
		sleep( 2);
	} else
		gps_state = GPS_SEARCH;

	// configure automatic reports to essential: limit messages to GGA, ZDA and PMTKANT
	gps_write(GPS_CMD_SETUP, sizeof(GPS_CMD_SETUP));

	pthread_create(&process_msg_t, NULL, process_msg, NULL);
}

/* places data read from the GPS in the message buffer for latter processing
 */
void output_from_gps(unsigned char* msg, int msg_len) {
	int end;

	// ignore messages sent before GPS is initialized
	if (gps_state == GPS_PRE_INIT)
		return;

	pthread_mutex_lock(&state_change_m);
	end = buff_end < buff_start ? buff_end + MSG_BUF_SIZE : buff_end;
	if ((MSG_BUF_SIZE - (end - buff_start)) > msg_len) { // we always leave one free position in the buffer
		// he have enough free space for the message
		for (end = 0; end < msg_len; end++) {
			msg_buf[(buff_end + end) % MSG_BUF_SIZE] = msg[end];
		}
		buff_end = (buff_end + end) % MSG_BUF_SIZE;
		pthread_cond_signal(&data_available_cond);
	} else {
		// should never happen. If it ever happens must rewrite to deal with it
		fprintf(debug_f,
				"No space left on buffer to hold message! Quitting.\n");
		exit(1);
	}
#ifdef GPS_DEBUG
	//	fprintf(debug_f, "Stored message - Size = %d. Buffer using from %d to %d\n", msg_len,
	//			buff_start, buff_end);
#endif
	pthread_mutex_unlock(&state_change_m);
}

char is_gps_ready() {
	return gps_state == GPS_READY; // && gps_fault == GPS_OK;
}

/* Get current time from the GPS device
 * Will return 1 if currently unable to read the time
 * Will return 2 if error occurred while waiting for time reply
 * Will return 0 if time is read
 * */
int getGPStimeUTC(struct timeval *tv) {
	int return_value = 0;

	pthread_mutex_lock(&state_change_m);
	if (gps_state != GPS_READY || current_time != NULL) { //|| gps_fault != GPS_OK
		pthread_mutex_unlock(&state_change_m);
		return 1;
	} else {
		current_time = tv;
		gps_state = GPS_QUERY;
		// wait for reply
		while (gps_state == GPS_QUERY)
			pthread_cond_wait(&reply_available_cond, &state_change_m);

		// gps received error message before being able to provide reply
		if (gps_state != GPS_READY)
			return_value = 2;

		current_time = NULL;
		pthread_mutex_unlock(&state_change_m);
		return return_value;
	}
}

/* Get current latitude, longitude and altitude from the GPS device
 * Will return 1 if currently unable to read the position
 * Will return 0 if position is read
 * Longitude and longitude in degrees (-180 to 180)
 * Altitude in meters
 * */
int getGPSLLA(double *latitude, double * longitude, double *altitude) {

	if( gps_state == GPS_READY) {
		pthread_mutex_lock(&state_change_m);
		*latitude = current_latitude;
		*longitude = current_longitude;
		*altitude = current_altitude;
		pthread_mutex_unlock(&state_change_m);
		return 0;
	} else
		return 1;
}

int getgpssatellites() {
	return num_satellites;
}

gps_fault_t getgpsfault() {
	return gps_fault;
}



/* verify the checksum of an NMEA message
 * msg must be a full NMEA message
 * Returns 1 if checksum matches and 0 otherwise
 * http://rietman.wordpress.com/2008/09/25/how-to-calculate-the-nmea-checksum/
 */
char valid_NMEA_checksum( char* msg) {
	int x;
	unsigned char checksum = 0;
	unsigned char provided_checksum = 0;

	for( x = 0; msg[x] != '*' && msg[x] != '\0'; x++)
		checksum ^= msg[x];

	if( msg[x] != '*')
		return 0;

	sscanf( msg + x + 1, "%hhX", &provided_checksum);

	//printf("%s\n*%d*%d*\n", msg, checksum, provided_checksum);
	return checksum == provided_checksum;
}

void* process_msg(void *unused) {
	int start, end;
	int x;

	pthread_mutex_lock(&state_change_m);
	while (1) {
		start = buff_start;
		end = buff_end;
		while (!lookup_packet(&start, &end)) {
			pthread_cond_wait(&data_available_cond, &state_change_m);
			start = buff_start;
			end = buff_end;
		}

		if (end < start)
			end += MSG_BUF_SIZE;

		if( valid_NMEA_checksum( inst_buf)) {
#ifdef GPS_DEBUG
			packet_content("Will process message.", start, end);
#endif
			if( !strncmp( inst_buf, GPS_MSG_ANTENNA, sizeof(GPS_MSG_ANTENNA)-1)) {
				// antenna status report
				switch( atoi( inst_buf+sizeof(GPS_MSG_ANTENNA))) {
				case 0:
					gps_fault = GPS_ANTENA_OPEN;
					break;
				case 1:
					gps_fault = GPS_OK;
					break;
				case 2:
					gps_fault = GPS_ANTENA_SHORT;
					break;
				default:
					packet_content(	"Unknown antenna status code!", start, end);
					exit(1);
				}

			}
			else if ( !strncmp( inst_buf, GPS_MSG_STATUS_TIME_POS, sizeof(GPS_MSG_STATUS_TIME_POS)-1)) {
				double lat_aux;
				double long_aux;

				//skip to latitude
				for( x=0; inst_buf[x] != ','; x++);
				for( x++; inst_buf[x] != ','; x++);
				lat_aux = atof( inst_buf + x +1);

				current_latitude = floor( lat_aux / 100.0) + fmod( lat_aux, 100.0) / 60.0;

				for( x++; inst_buf[x] != ','; x++);
				if( inst_buf[ x + 1] == 'S')
					current_latitude = -current_latitude;

				//skip to longitude
				for( x++; inst_buf[x] != ','; x++);
				long_aux = atof( inst_buf + x + 1);

				current_longitude = floor( long_aux / 100.0) + fmod( long_aux, 100.0) / 60.0;

				for( x++; inst_buf[x] != ','; x++);
				if( inst_buf[ x + 1] == 'W')
					current_longitude = -current_longitude;

				//skip to GPS quality indicator
				for( x++; inst_buf[x] != ','; x++);
				switch( inst_buf[ x + 1]) {
				case '0':
					gps_state = GPS_SEARCH;
					if (gps_state == GPS_QUERY)
						// will never receive reply for current time request
						pthread_cond_signal(&reply_available_cond);
					break;
				case '1':
				case '2':
					gps_state = GPS_READY;
					break;
				default:
					packet_content(	"Unknown GPS quality indicator code!", start, end);
					exit(1);
				}

				//skip to number of satellites in use
				for( x++; inst_buf[x] != ','; x++);
				num_satellites = atoi( inst_buf + x + 1);

				//skip to altitude
				for( x++; inst_buf[x] != ','; x++);
				for( x++; inst_buf[x] != ','; x++);
				current_altitude = atof( inst_buf + x + 1);
			}
			else if ( !strncmp( inst_buf, GPS_MSG_TIME, sizeof(GPS_MSG_TIME)-1)) {
				if (current_time) {
					// process time message
					struct tm broken_time;

					//skip to hours
					for( x=0; inst_buf[x] != ','; x++);

					sscanf( inst_buf + x + 1, "%2d%2d%2d", &broken_time.tm_hour,
							&broken_time.tm_min, &broken_time.tm_sec);

					for( x++; inst_buf[x] != ','; x++);
					broken_time.tm_mday = atoi( inst_buf + x + 1);

					for( x++; inst_buf[x] != ','; x++);
					broken_time.tm_mon = atoi( inst_buf + x + 1);

					for( x++; inst_buf[x] != ','; x++);
					broken_time.tm_year = atoi( inst_buf + x + 1)-1900;

					printf("%d:%d:%d %d/%d/%d\n", broken_time.tm_hour, broken_time.tm_min, broken_time.tm_sec, broken_time.tm_mday, broken_time.tm_mon, broken_time.tm_year);
					current_time->tv_usec = 0;
					current_time->tv_sec = mktime( &broken_time);

					gps_state = GPS_READY;
					pthread_cond_signal(&reply_available_cond);
				}
			}
			else {
				packet_content(	"Ignored message", start, end);
			}
		}
		else {
#ifdef GPS_DEBUG
			packet_content("Message failed checksum. Will be ignored.", start, end);
#endif
		}
		buff_start = end % MSG_BUF_SIZE;
	}

	return NULL;
}

