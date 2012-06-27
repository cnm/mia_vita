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

#include "macros.h"
#include "list.h"

char* output_binary_file = "/tmp/manel/miavita.bin";
char* output_json_file = "/tmp/manel/miavita.json";
int bin_fd = -1, json_fd = -1;

list* mklist(uint32_t capacity, char* new_filename)
{
  list* l = alloc(list, 1);
  l->buff = alloc(packet_t, capacity);
  l->rotate_at = capacity;
  l->new_filename = new_filename;
  l->lst_size = 0;

  return l;
}

void clear_list(list* l){
  l->lst_size = 0;
}

void rmlist(list* l){
    if(l)
      {
        free(l->buff);
        free(l);
      }
}

static void write_bin(packet_t pkt){
    int32_t to_write = sizeof(pkt), status, written = 0;
    while(written < to_write)
      {
        status = write(bin_fd, ((char*) &pkt) + written, to_write - written);
        if(status == -1)
          {
            perror("Unable to write binary data.\n");
            return;
          }
        written += status;
      }
}

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define sample_to_le(S)				\
  do{						\
      uint8_t* ___s = (uint8_t*) (S);		\
      uint8_t ___t[3] = {0};			\
      memcpy(___t, ___s, 3);			\
      ___s[0] = ___t[2];				\
      ___s[2] = ___t[0];				\
  }while(0)
#else
#define sample_to_le(S)
#endif

#warning "This implementation is not converting to correct endianess. Only works if everything is ARM. (little endian)"
static packet_t ntohpkt(packet_t pkt)
{
#ifdef __GPS__
    pkt.gps_us = be64toh( pkt.gps_us );
#endif
    pkt.timestamp = be64toh( pkt.timestamp );
    pkt.air = be64toh( pkt.air );
    pkt.seq = be32toh( pkt.seq );
    return pkt;
}

static void write_json(packet_t pkt)
{
  static uint8_t first = 1;
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

 *(sample1_byte + 0) = pkt.samples[0 + 2];
 *(sample1_byte + 1) = pkt.samples[0 + 1];
 *(sample1_byte + 2) = pkt.samples[0 + 0];

 *(sample2_byte + 0) = pkt.samples[4 + 1];
 *(sample2_byte + 1) = pkt.samples[4 + 0];
 *(sample2_byte + 2) = pkt.samples[0 + 3];

 *(sample3_byte + 0) = pkt.samples[8 + 0];
 *(sample3_byte + 1) = pkt.samples[4 + 3];
 *(sample3_byte + 2) = pkt.samples[4 + 2];

 *(sample4_byte + 0) = pkt.samples[8 + 3];
 *(sample4_byte + 1) = pkt.samples[8 + 2];
 *(sample4_byte + 2) = pkt.samples[8 + 1];

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

  pkt = ntohpkt(pkt);

  if(!first)
    {
      write(json_fd, ",\n", 2);
    }
  else
    {
      write(json_fd, "\n", 1);
      first = 0;
    }

#ifdef __GPS__
  if(test)
    {
      struct timeval t;
      gettimeofday(&t, NULL);

      pkt.timestamp = (((int64_t) t.tv_sec) * 1000000 + t.tv_usec) - pkt.timestamp;
      pkt.gps_us = get_millis_offset() - pkt.gps_us;
    }
  sprintf(buff"\"%u:%u\" : {\"gps_us\" : %lld, \"timestamp\" : %lld, \"air_time\" : %lld, \"sequence\" : %u, \"fails\" : %u, \"retries\" : %u, \"sample_1\" : %05d, \"sample_2\" : %05d, \"sample_3\" : %05d, \"sample_4\" : %05d \"node_id\" : %u }", pkt.id, pkt.seq, pkt.gps_us, pkt.timestamp, pkt.air, pkt.seq, pkt.fails, pkt.retries, sample1, sample2, sample3, sample4, pkt.id);
#else
  sprintf(buff, "\"%u:%u\" : {\"timestamp\" : %lld, \"air_time\" : %lld, \"sequence\" : %u, \"fails\" : %u, \"retries\" : %u, \"sample_1\" : %05d, \"sample_2\" : %05d, \"sample_3\" : %05d, \"sample_4\" : %05d \"node_id\" : %u }", pkt.id, pkt.seq, pkt.timestamp, pkt.air, pkt.seq, pkt.fails, pkt.retries, sample1, sample2, sample3, sample4, pkt.id);
#endif
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

static uint8_t open_output_files()
{
  bin_fd = open(output_binary_file, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if(bin_fd == -1)
    {
      perror("Unable to open binary output file");
      return 0;
    }

  json_fd = open(output_json_file, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if(json_fd == -1)
    {
      close(bin_fd);
      perror("Unable to open json output file");
      return 0;
    }
  write(json_fd, "{", 1);
  return 1;
}

static void close_output_files()
{
  close(bin_fd);
  write(json_fd, "\n}", 2);
  close(json_fd);
}

static void dump(list* l)
{
  char buff[100] = {0};
  uint32_t i;

  sprintf(buff, "%s.bin", l->new_filename);
  rename(output_binary_file, buff);

  memset(buff, 0, sizeof(buff));
  sprintf(buff, "%s.json", l->new_filename);
  rename(output_json_file, buff);

  if(open_output_files())
    {
      for(i = 0; i < l->lst_size; i++)
        {
          write_bin(l->buff[i]);
          write_json(l->buff[i]);
        }
      close_output_files();
    }
}

#warning "This implementation of __packet_comparator has bugs. It is not receiving the correct memory address of a and b."
//Because timestamp is int64_t and this should return int we cannot just do a.timestamp - b.timestamp.
int  __packet_comparator(const void* a, const void* b){
    int64_t at = ((packet_t*) a)->timestamp;
    int64_t bt = ((packet_t*) b)->timestamp;

#ifdef __DEBUG__
    printf("A seq: %u\n", ((packet_t*) a)->seq);
#endif

    if(at < bt){
#ifdef __DEBUG__
        printf("%lld < %lld\n", at, bt);
#endif
        return -1;
    }

    if(at > bt){
#ifdef __DEBUG__
        printf("%lld > %lld\n", at, bt);
#endif
        return 1;
    }

#ifdef __DEBUG__
    printf("%lld == %lld\n", at, bt);
#endif
    return 0;
}

void insert(list* l, packet_t* p)
{
  memcpy(l->buff + l->lst_size, p, sizeof(*p));
  l->lst_size++;
  if(l->rotate_at == l->lst_size)
    {
      dump(l);
      clear_list(l);
    }
}