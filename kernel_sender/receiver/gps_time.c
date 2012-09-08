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

#include "gps_time.h"

#define PI 3.1415926535898

#define DLE 0x10
#define ETX 0x03

#define GPS_REQUEST_TIME 0x21

#define GPS_CMD_RESET 0x25
#define GPS_CMD_AUTOMATIC_POS_REPORT 0x35
#define GPS_CMD_ENHANCED_SENSITIVITY 0x69
#define GPS_CMD_REQ_HEALTH 0x26
#define GPS_CMD_REQ_POS 0x37

#define GPS_MSG_TIME 0x41
#define GPS_MSG_SW_VERSION 0x45
#define GPS_MSG_RECEIVER_HEALTH 0x46
#define GPS_MSG_LLA 0x4A
//#define GPS_MSG_ 0x4B
//#define GPS_MSG_ 0x6D
//#define GPS_MSG_ 0x82

#define SEC_WEEK 604800
#define SEC_GPS_EPOCH 315964800

gps_fault_t gps_fault = GPS_OK;

char reset_msg[] = { DLE, GPS_CMD_RESET, DLE, ETX };
// set latitude longitude altitude (LLA) position reports
char config_auto_report_msg[] = { DLE, GPS_CMD_AUTOMATIC_POS_REPORT, 2, 0, 1, 0, DLE, ETX };
char enhance_sensitivity_on_msg[] = { DLE, GPS_CMD_ENHANCED_SENSITIVITY, 1, 0,
    DLE, ETX };
char enhance_sensitivity_off_msg[] = { DLE, GPS_CMD_ENHANCED_SENSITIVITY, 0, 0,
    DLE, ETX };
char request_time_msg[] = { DLE, GPS_REQUEST_TIME, DLE, ETX };
char request_health_msg[] = { DLE, GPS_CMD_REQ_HEALTH, DLE, ETX};
char request_position_msg[] = { DLE, GPS_CMD_REQ_POS, DLE, ETX};

int num_satellites = 0;

#define LHEX(x) (x & 0x0F)
#define HHEX(x) (x >> 4)
#define BIT(x,y) ( (x >> y) & 0x01)

/* enables debug messages */
#define GPS_DEBUG 1

#define MSG_BUF_SIZE 1024

/** circular buffer for storing messages from the GPS for processing */
unsigned char msg_buf[MSG_BUF_SIZE];
/** first (inclusive) and last (exclusive) positions of data in the circular buffer */
int buff_start = 0, buff_end = 0;

/* Lock for acessing GPS state and circular buffer */
pthread_mutex_t state_change_m = PTHREAD_MUTEX_INITIALIZER;
/* lock for writting messages to the GPS */
pthread_mutex_t gps_write_m = PTHREAD_MUTEX_INITIALIZER;

/* notify message handling thread that new data is available */
pthread_cond_t data_available_cond = PTHREAD_COND_INITIALIZER;
/* notify time request handling thread that time reply is available */
pthread_cond_t reply_available_cond = PTHREAD_COND_INITIALIZER;

// structure to store time reply
struct timeval *current_time = NULL;

// variables for storing current possition
float *current_latitude = NULL;
float *current_longitude = NULL;
float *current_altitude = NULL;

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

void pretty_print_packet(int start, int end)
{
  switch(msg_buf[start % MSG_BUF_SIZE])
    {
    case 0x41:
      fprintf(debug_f, "\t GPS Time\n");
      break;
    case 0x45:
      fprintf(debug_f, "\t  Software Version Information\n");
      break;
    case 0x46:
      fprintf(debug_f, "\t Health of Receiver\n");
      break;
    case 0x4A:
      fprintf(debug_f, "\t Single-Precision LLA Position Fix and Bias Information\n");
      break;
    case 0x4B:
      fprintf(debug_f, "\t Machine/Code ID and Additional Status\n");
      break;
    case 0x56:
      fprintf(debug_f, "\t Velocity Fix, East-North-Up\n");
      break;
    case 0x57:
          fprintf(debug_f, "\t Information About Last Computed Fix\n");
	  break;
    case 0x6D:
      fprintf(debug_f, "\t All-In-View Satellite Selection\n");
      break;
    case 0x82:
      fprintf(debug_f, "\t Differential Position Fix Mode\n");
      break;
    case 0x84:
          fprintf(debug_f, "\t Double-Precision LLA Position Fix and Bias Information\n");
	  break;
    default:
    	  fprintf(debug_f, "\t Unknown\n");
    }
}

void packet_content(char *error_msg, int start, int end) {
    int x;

    fprintf(debug_f, "Packet content %d to %d. %s: ", start, end, error_msg);

#ifdef GPS_DEBUG
    pretty_print_packet(start, end);
#endif

    for (x = start; x != end; x++)
      fprintf(debug_f, "%hhX ", msg_buf[x % MSG_BUF_SIZE]);

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
    int dle_count = 0;
    char packet_found = 0;
    int origin;

    // "make" circular buffer linear
    if (*end < *start)
      *end += MSG_BUF_SIZE;

    // do we have enough data for a message?
    if (*end - *start < 4)
      return 0;

    // does buffer start with a message?
    if (msg_buf[*start] != DLE) {
        //if (gps_state != GPS_RESET) {
        packet_content( "Packet does not start with DLE. Should never happen rewrite code!",
                       *start, *end);
        exit(1);
        //} else {
        // there may be garbage in the GPS data stream, search for first packet
        //}
    }

    // look for message end
    for (pos = *start; pos < *end && !packet_found; pos++) {
        switch (msg_buf[pos % MSG_BUF_SIZE]) {
          case DLE:
            dle_count++;
            break;
          case ETX:
            if (dle_count % 2)
              packet_found = 1;
          default:
            dle_count = 0;
        }
    }

    if (!packet_found)
      return 0;

    // a packet was found. Let's remove header, trailer and padding
    *end = pos % MSG_BUF_SIZE;
    pos = pos - 1;
    origin = pos - 2; // trailler removed

    while (origin > *start) { // header removed
        // shift message towards the end
        msg_buf[pos % MSG_BUF_SIZE] = msg_buf[origin % MSG_BUF_SIZE];

        // remove padding
        if (msg_buf[origin % MSG_BUF_SIZE] == DLE && msg_buf[(origin - 1)
            % MSG_BUF_SIZE] == DLE)
          origin -= 2;
        else
          origin--;

        pos--;
    }
    *start = (pos + 1) % MSG_BUF_SIZE;
    return 1;
}

void init_gps(char indoor, char reset, void(*input_for_gps)(char *msg, int msg_len),
              FILE *output_status_func) {
    debug_f = output_status_func;

    gps_write = input_for_gps;

    if( indoor && !reset)
    	fprintf(debug_f, "Warning, indoor mode requested but device not reset\n");

    pthread_mutex_lock(&gps_write_m);
    if( reset) {
    	// enable enhanced sensitivity for indoor operation
    	if (indoor)
    		gps_write(enhance_sensitivity_on_msg, sizeof(enhance_sensitivity_on_msg));
    	else
    		gps_write(enhance_sensitivity_off_msg, sizeof(enhance_sensitivity_off_msg));

    	// reset gps device
		gps_write(reset_msg, sizeof(reset_msg));
		gps_state = GPS_RESET;
    }
    else
    	gps_state = GPS_SEARCH;

    // configure automatic position and velocity reports
    gps_write(config_auto_report_msg, sizeof(config_auto_report_msg));

    if( !reset)
    	// request health message now instead of awaiting for an automatic message (which takes up to 5s)
    	gps_write(request_health_msg, sizeof(request_health_msg));

    pthread_mutex_unlock(&gps_write_m);

    pthread_create(&process_msg_t, NULL, process_msg, NULL);
}

/* places data read from the GPS in the message buffer for latter processing
 */
void output_from_gps(unsigned char* msg, int msg_len) {
    int end;

    // ignore messages sent before GPS is initialised
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
        fprintf(debug_f, "No space left on buffer to hold message! Quitting.\n");
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
        // send time request
        pthread_mutex_lock(&gps_write_m);
        gps_write(request_time_msg, sizeof(request_time_msg));
        pthread_mutex_unlock(&gps_write_m);

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
 * Will return 2 if error occurred while waiting for position reply
 * Will return 0 if position is read
 * Longitude and longitude in degrees (-180 to 180)
 * Altitude in meters
 * */
int getGPSLLA(float *latitude, float * longitude, float *altitude) {
    int return_value = 0;

    pthread_mutex_lock(&state_change_m);
    if (gps_state != GPS_READY || current_latitude != NULL) { // || gps_fault != GPS_OK 
        pthread_mutex_unlock(&state_change_m);
        return 1;
    } else {
        // send position request
        pthread_mutex_lock(&gps_write_m);
        gps_write(request_position_msg, sizeof(request_position_msg));
        pthread_mutex_unlock(&gps_write_m);

        current_latitude = latitude;
        current_longitude = longitude;
        current_altitude = altitude;

        gps_state = GPS_QUERY;
        // wait for reply
        while (gps_state == GPS_QUERY)
          pthread_cond_wait(&reply_available_cond, &state_change_m);

        // gps received error message before being able to provide reply
        if (gps_state != GPS_READY)
          return_value = 2;

        current_latitude = NULL;
        pthread_mutex_unlock(&state_change_m);
        return return_value;
    }
}

int getgpssatellites() {
    return num_satellites;
}

gps_fault_t getgpsfault() {
    return gps_fault;
}

void* process_msg(void *unused) {
    int start, end;
    char byte;
    char number_buf[sizeof(double)];
    int x;
    float time_sec;
    short week_num;

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

#ifdef GPS_DEBUG
        /*        fprintf(debug_f, "Will process new message - Size = %d, from %d to %d\n",*/
        /*               end - start, start, end);*/
        packet_content("found this ", start, end);
#endif

        //handle non-state specific messages
        switch (msg_buf[start]) {
          case GPS_MSG_RECEIVER_HEALTH:
            if (msg_buf[(start + 1) % MSG_BUF_SIZE] == 0x02) {
                // reset gps device
                pthread_mutex_lock(&gps_write_m);
                gps_write(reset_msg, sizeof(reset_msg));
                pthread_mutex_unlock(&gps_write_m);

                if (gps_state == GPS_QUERY) {
                    // will never receive reply for current time request
                    pthread_cond_signal(&reply_available_cond);
                }

                gps_fault = GPS_NEEDED_RESET;
                gps_state = GPS_RESET;
            } else
              gps_fault = GPS_OK;

            byte = msg_buf[(start + 2) % MSG_BUF_SIZE];
            if (HHEX( byte) == 0x03)
              gps_fault = GPS_ANTENA_SHORT;
            else if (HHEX( byte) == 0x01)
              gps_fault = GPS_ANTENA_OPEN;
            else if (BIT( byte, 0))
              gps_fault = GPS_BATTERY_FAULT;

            break;
        }

        // handle state specific messages
        if (gps_state == GPS_RESET) {
            // wait for SW version message after reset.
            if (msg_buf[start] == GPS_MSG_SW_VERSION) {
                gps_state = GPS_SEARCH;
#ifdef GPS_DEBUG
                fprintf(debug_f, "*\nReset completed, entering satellite search mode.\n");
#endif
            }
        } else if (gps_state == GPS_SEARCH) {
            switch (msg_buf[start]) {
              case GPS_MSG_RECEIVER_HEALTH:
                if (msg_buf[(start + 1) % MSG_BUF_SIZE] == 0) {
                    gps_state = GPS_READY;
#ifdef GPS_DEBUG
                    fprintf(debug_f, "*\nGPS fix obtained.\n");
#endif
                }
                break;
            }
        } else if (gps_state == GPS_READY) {
            switch (msg_buf[start]) {
              case GPS_MSG_RECEIVER_HEALTH:
                if (msg_buf[(start + 1) % MSG_BUF_SIZE] != 0) {
                    gps_state = GPS_SEARCH;
#ifdef GPS_DEBUG
                    fprintf(debug_f, "*\nGPS fix LOST. Searching for satellites.\n");
#endif
                }
                break;
            }
        } else {
            // GPS_QUERY
            switch (msg_buf[start]) {
              case GPS_MSG_RECEIVER_HEALTH:
                if (msg_buf[(start + 1) % MSG_BUF_SIZE] != 0) {

                    gps_state = GPS_SEARCH;
#ifdef GPS_DEBUG
                    fprintf(debug_f, "*\nGPS fix LOST while waiting for TIME REPLY. Searching for satellites.\n");
#endif

                    pthread_cond_signal(&reply_available_cond);
                }
                break;
              case GPS_MSG_TIME:
            	if( !current_time)
            		break;
                // process time reply
#ifdef GPS_DEBUG
            	  fprintf(debug_f, "*\nReceived time.\n");
#endif
                // GPS time of week
                for (x = 0; x < sizeof(float); x++) {
                    number_buf[x] = msg_buf[(start + sizeof(float) - x)
                                           % MSG_BUF_SIZE];
                }
                time_sec = *((float *) number_buf);

                // GPS time offset
                for (x = 0; x < sizeof(float); x++) {
                    number_buf[x] = msg_buf[(start + 6 + sizeof(float) - x)
                                           % MSG_BUF_SIZE];
                }
                time_sec -= *((float *) number_buf);

                // week number
                for (x = 0; x < sizeof(short); x++) {
                	number_buf[x] = msg_buf[(start + 4 + sizeof(short) - x)
                	                       % MSG_BUF_SIZE];
				}
                week_num = *((short *) number_buf);

                current_time->tv_sec = SEC_GPS_EPOCH + ((long) SEC_WEEK) * week_num + ( (long) time_sec);
                current_time->tv_usec =  (int) ((time_sec - (int) time_sec) * 1000000.0);
                gps_state = GPS_READY;
                pthread_cond_signal(&reply_available_cond);
                break;
              case GPS_MSG_LLA:
            	  if( !current_latitude)
            		  break;
            	  // process time reply
#ifdef GPS_DEBUG
            	  fprintf(debug_f, "*\nReceived position.\n");
#endif
				  // GPS latitude
				  for (x = 0; x < sizeof(float); x++) {
					  number_buf[x] = msg_buf[(start + sizeof(float) - x)
											 % MSG_BUF_SIZE];
				  }
				  *current_latitude = *((float *) number_buf) / PI * 180;

				  // GPS longitude
				  for (x = 0; x < sizeof(float); x++) {
					  number_buf[x] = msg_buf[(start + 4 + sizeof(float) - x)
											 % MSG_BUF_SIZE];
				  }
				  *current_longitude = *((float *) number_buf) / PI * 180;

				  // GPS altitude
				  for (x = 0; x < sizeof(float); x++) {
					  number_buf[x] = msg_buf[(start + 8 + sizeof(float) - x)
											 % MSG_BUF_SIZE];
				  }
				  *current_altitude = *((float *) number_buf);

				  gps_state = GPS_READY;
				  pthread_cond_signal(&reply_available_cond);
            	  break;
            }
        }

        buff_start = end % MSG_BUF_SIZE;
    }
}

