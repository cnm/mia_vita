#ifndef __LIST_H__
#define __LIST_H__

#include <stdint.h>

#include "miavita_packet.h"

typedef struct{
  uint32_t lst_size;
  uint32_t rotate_at;
  packet_t* buff;
  char* new_filename;
}list;

extern char* output_binary_file;
extern char* output_json_file;

extern list* mklist(uint32_t capacity, char* new_filename);
extern void rmlist(list* l);
extern void insert(list* l, packet_t* p);
#endif