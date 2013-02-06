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

/* char* output_binary_file = "/tmp/manel/miavita.bin"; */
/* char* output_json_file = "/tmp/manel/miavita.json"; */
char* output_binary_file = "miavita.bin";
char* output_json_file = "miavita.json";
char* archive_json_file = "miavita.json.archive";
int last_packet[16];
int64_t last_arrival_interval[16];
int64_t last_arrival_time[16];

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


#if __BYTE_ORDER == __LITTLE_ENDIAN
#define sample_to_le(S)               \
  do{                                 \
      uint8_t* ___s = (uint8_t*) (S); \
      uint8_t ___t[3] = {0};          \
      memcpy(___t, ___s, 3);          \
      ___s[0] = ___t[2];              \
      ___s[2] = ___t[0];              \
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

static void write_json(packet_t pkt, uint8_t first, int json_fd )
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
  sprintf(buff, "\"%u:%u\" : {\"gps_us\" : %lld, \"timestamp\" : %lld, \"air_time\" : %lld, \"sequence\" : %u, \"fails\" : %u, \"retries\" : %u, \"sample_1\" : %d, \"sample_2\" : %d, \"sample_3\" : %d, \"sample_4\" : %d, \"node_id\" : %u }", pkt.id, pkt.seq, pkt.gps_us, pkt.timestamp / 100, pkt.air, pkt.seq, pkt.fails, pkt.retries, sample1, sample2, sample3, sample4, pkt.id);
#else
  sprintf(buff, "\"%u:%u\":{\"ts\":%lld,\"1\":%d,\"2\":%d,\"3\":%d,\"4\": %d}", pkt.id, pkt.seq, pkt.timestamp, sample1, sample2, sample3, sample4);
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

static uint8_t open_output_files(char * output_filename)
{
  /* Use this to truncate */
  /* int json_fd = open(output_filename, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH); */
  /* Use this to append */
  int json_fd = open(output_filename, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

  if(json_fd == -1)
    {
      perror("Unable to open json output file");
      return 0;
    }
  write(json_fd, "{", 1);
  return json_fd;
}

static void close_output_files(int json_fd, char * temp_path, char * new_path)
{
  write(json_fd, "\n}", 2);
  close(json_fd);

  rename(temp_path, new_path);
}

static void dump(list* l)
{
  uint32_t i;
  int json_fd;
  #warning 326 is not a logical number. Had to use it as 6 (the supossed correct value) gave a seg fault. REDO URGENT
  char * temp_path = (char *) calloc (sizeof(l->new_filename) + 326 * sizeof(char), sizeof(char));
  sprintf(temp_path, "%s.temp", l->new_filename);

  if((json_fd = open_output_files(temp_path)))
    {
      write_json(l->buff[0], 1, json_fd);
      for(i = 1; i < l->lst_size; i++)
        {
          write_json(l->buff[i], 0, json_fd);
        }
      close_output_files(json_fd, temp_path, l->new_filename);
    }
  free(temp_path);
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

#define SEC_2_USEC 1000000L
#define USEC_2_NSEC 1000
int64_t get_kernel_current_time(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return ((int64_t) tv.tv_sec) * SEC_2_USEC + ((int64_t) tv.tv_usec);
}

void insert(list* l, packet_t* p)
{
  int64_t current_time, current_interval;

  //Check if node id > 0 && < 16
  if(p->id > 0 && p->id < 16){

    // Let's insert the jitter of packet p
    current_time = get_kernel_current_time();
    current_interval = current_time - last_arrival_time[p->id];
    last_arrival_time[p->id] = current_time;

    /* Retries is actually jitter in milliseconds */
    p->retries = abs(current_interval - last_arrival_interval[p->id]) / 1000;
    last_arrival_interval[p->id] = p->retries;

    // Calculate missing packets
    p->fails = p->seq - last_packet[p->id];
    last_packet[p->id] = p->seq;
  }

  memcpy(l->buff + l->lst_size, p, sizeof(*p));
  l->lst_size++;
  if(l->rotate_at == l->lst_size)
    {
      dump(l);
      clear_list(l);
    }
}
