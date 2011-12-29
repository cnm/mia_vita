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

typedef struct __attribute__ ((__packed__)){ // specifies that the minimum required memory be used to represent the type.

#ifdef __GPS__
  int64_t gps_us;                            // where to store the GPS time
#endif

  int64_t timestamp;                         // Signed Transmission time since packet was created
  int64_t air;                               // Estimated time the packet was "on the air"
  uint32_t seq;                              // Sequence number
  uint8_t fails;                             // Fails since last packet was received
  uint8_t retries;                           // Retries ...
  uint8_t samples[12];                       // Samples for the four channels
  uint8_t id;                                // ID of the originator node
}packet_t;

#endif
