/*
 *@Author Frederico Gonçalves [frederico.lopes.goncalves@gmail.com]
 *
 * This file contains the packet used to transmit data from the geophone to the sink node.
 *
 * As you may notice it has a conditional compilation. Passing -D__GPS__ to gcc will compile the code
 * for GPS test scenarios. It is important to notice that this has nothing to due with the fact that
 * every node uses or not GPS devices. This is only to test the synchronization protocol against GPS
 * devices. If every node in the network uses GPS and its time should be used, then you should change the code
 * to call the GPS time functions and use the field 'timestamp'.
 *
 * Furthermore, compiling this code with -D__GPS__ will also require compiling the server side with the same flag!!!!
 */

#ifndef __MIAVITA_PACKET_H__
#define __MIAVITA_PACKET_H__

#include <linux/kernel.h> /*uint32_t, int32_t, etc.*/

typedef struct __attribute__ ((__packed__)){

#ifdef __GPS__
  int64_t gps_us;
#endif

  int64_t timestamp;
  int64_t air;
  uint32_t seq;
  uint8_t fails;
  uint8_t retries;
  uint8_t samples[12];
  uint8_t id;
}packet_t;

#endif
