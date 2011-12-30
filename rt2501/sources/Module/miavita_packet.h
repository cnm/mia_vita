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

typedef struct __attribute__ ((__packed__))  // specifies that the minimum required memory be used to represent the type.
{

#ifdef __GPS__
    int64_t gps_us;                            // where to store the GPS time
#endif

    int64_t timestamp;                         // Signed Transmission time the packet was created (1) or the time since packet was created (2)
    int64_t air;                               // Estimated time the packet was "on the air"
    uint32_t seq;                              // Sequence number
    uint8_t fails;                             // Fails since last packet was received
    uint8_t retries;                           // Retries ...
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
