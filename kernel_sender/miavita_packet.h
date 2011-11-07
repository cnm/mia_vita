#ifndef __MIAVITA_PACKET_H__
#define __MIAVITA_PACKET_H__

#include <linux/kernel.h> /*uint32_t, int32_t, etc.*/

typedef struct {
  int64_t timestamp;
  int64_t air;
  uint32_t seq;
  uint8_t fails;
  uint8_t retries;
  uint8_t samples[9];
  uint8_t id;
}packet_t;

#endif
