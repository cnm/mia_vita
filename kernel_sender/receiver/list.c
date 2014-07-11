/* TODO - This file was to be completelly rewritten. It has been adapted for to long to changing requirements.
 * It is trying to do stuff for which it is no longer necessary (we use GPS devices now) and is incredibly difficult to maintain.
 * */


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <endian.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>

#include "macros.h"
#include "list.h"


/**
 * @brief Changes from network order to host.
 * It has to be the inverse of what happend in kernel_sender/sender_kthread.c:prepare_packet
 *
 * @param pkt - The packet to change the variables to host order. Caution it changes the packet itself
 */
static void ntohpkt(packet_t *pkt)
{
  pkt->timestamp = be64toh( pkt->timestamp );
  pkt->seq = be32toh( pkt->seq );
}

/**
 * @brief Write a packet to a file
 *
 * @param pkt
 * @param first - Indicates if it is the first line to be written. (shouldn't it be last?)
 * @param json_fd - file descriptor for the output file
 */
void write_json(packet_t *pkt, uint8_t first, int json_fd )
{
  /* static uint8_t first = 1; */
  char buff[2048] = {0};
  uint32_t to_write, written = 0, status;
  uint32_t sample1 = 0, sample2 = 0, sample3 = 0, sample4 = 0;
  uint8_t * sample1_byte;
  uint8_t * sample2_byte;
  uint8_t * sample3_byte;
  uint8_t * sample4_byte;

  sample1_byte = (uint8_t *) &sample1;
  sample2_byte = (uint8_t *) &sample2;
  sample3_byte = (uint8_t *) &sample3;
  sample4_byte = (uint8_t *) &sample4;

  *(sample1_byte + 0) = pkt->samples[0 + 2];
  *(sample1_byte + 1) = pkt->samples[0 + 1];
  *(sample1_byte + 2) = pkt->samples[0 + 0];

  *(sample2_byte + 0) = pkt->samples[4 + 1];
  *(sample2_byte + 1) = pkt->samples[4 + 0];
  *(sample2_byte + 2) = pkt->samples[0 + 3];

  *(sample3_byte + 0) = pkt->samples[8 + 0];
  *(sample3_byte + 1) = pkt->samples[4 + 3];
  *(sample3_byte + 2) = pkt->samples[4 + 2];

  *(sample4_byte + 0) = pkt->samples[8 + 3];
  *(sample4_byte + 1) = pkt->samples[8 + 2];
  *(sample4_byte + 2) = pkt->samples[8 + 1];

  if(sample1 > 0x800000){
      sample1 = (((~sample1)+1) & 0x00FFFFFF)*(-1);
  }
  if(sample2 > 0x800000){
      sample2 = (((~sample2)+1) & 0x00FFFFFF)*(-1);
  }
  if(sample3 > 0x800000){
      sample3 = (((~sample3)+1) & 0x00FFFFFF)*(-1);
  }
  if(sample4 > 0x800000){
      sample4 = (((~sample4)+1) & 0x00FFFFFF)*(-1);
  }

  // Changes the byteorder
  ntohpkt(pkt);

  if(!first)
    {
      write(json_fd, ",\n", 2);
    }
  else
    {
      write(json_fd, "\n", 1);
      first = 0;
    }

  sprintf(buff, "\"%u:%u\":{\"ts\":%llu,\"1\":%d,\"2\":%d,\"3\":%d,\"4\": %d}",
          pkt->id, pkt->seq, pkt->timestamp, sample1, sample2, sample3, sample4);

  to_write = strlen(buff);
  while(written < to_write)
    {
      status = write(json_fd, buff + written, to_write - written);
      if(status == -1)
        {
          perror("Unable to write to json file");
          return;
        }
      written += status;
    }
}
