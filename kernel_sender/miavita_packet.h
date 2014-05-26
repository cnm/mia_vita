/*
 *@Author Frederico Gonçalves [frederico.lopes.goncalves@gmail.com]
 *
 * This file contains the packet used to transmit data from the geophone to the sink node.
 */

#ifndef __MIAVITA_PACKET_H__
#define __MIAVITA_PACKET_H__

typedef struct __attribute__ ((__packed__)){ // specifies that the minimum required memory be used to represent the type.
  int64_t timestamp;                         // Signed Transmission time the packet was created 
  uint32_t seq;                              // Sequence number
  uint8_t samples[12];                       // Samples for the four channels
  uint8_t id;                                // ID of the originator node
} packet_t;

/*
 *  +---------+                                             +---------+                         +---------+
 *  |   App   |  (1)        (Kernel Module)                 |   App   |                     (1) |   App   |
 *  |---------|   |                                         |---------|                      ↑  |---------|
 *  |  Trans  |   |         (UDP)                           |  Trans  |                      |  |  Trans  |
 *  |---------|   |                                         |---------|                      |  |---------|
 *  |  Inter  |   |         (IP)                            |  Inter  |                      |  |  Inter  |
 *  |---------|   |                                         |---------|                      |  |---------|
 *  |         |   ↓                                       ↔ |         | ↔                    |  |         |
 *  |  Link   |  (1->2)     (MAC - Kernel Module)    (2->1) |  Link   | (1->2)           (2->1) |  Link   |
 *  |         |   --------------------------------------->  |         | --------------------->  |         |
 *  +---------+                                             +---------+                         +---------+
 *
 *
 * 1    )  Happens at the interruption at proc_entry.c  (function: read_four_channels in interruption/fpga.c)
 * 1-2  )  Happens in the rt2501 module                 (function: synch_out_data_packet in rt2501/sources/Module/sync_proto.c)
 * 2-1  )  Happens at the rt2501 module                 (function: synch_in_data_packet  in rt2501/sources/Module/sync_proto.c)
 */
#endif
