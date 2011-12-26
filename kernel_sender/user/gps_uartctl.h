/*
 * gps_uartctl.h
 *
 *  Created on: Jun 13, 2011
 *      Author: ricardo
 */

#ifndef GPS_UARTCTL_H_
#define GPS_UARTCTL_H_

void uart_init( char indoor, char reset, FILE *status_output, char* gps_dev);

void uart_write( char *msg, int msg_len);

#endif /* GPS_UARTCTL_H_ */
