#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <endian.h>

#include "macros.h"
#include "list.h"

char* output_binary_file = "miavita.bin";
char* output_json_file = "miavita.json";
int bin_fd = -1, json_fd = -1;

list* mklist(uint32_t capacity, char* new_filename){
  list* l = alloc(list, 1);
  l->buff = alloc(packet_t, capacity);
  l->rotate_at = capacity;
  l->new_filename = new_filename;
}

void rmlist(list* l){
  if(l){
    free(l->buff);
    free(l);
  }
}

static void write_bin(packet_t pkt){
  int32_t to_write = sizeof(pkt), status, written = 0;
  while(written < to_write){
    status = write(bin_fd, ((char*) &pkt) + written, to_write - written);
    if(status == -1){
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

static packet_t ntohpkt(packet_t pkt){
#ifdef __GPS__
  pkt.gps_us = be64toh( pkt.gps_us );
#endif
  pkt.timestamp = be64toh( pkt.timestamp );
  pkt.air = be64toh( pkt.air );
  pkt.seq = be32toh( pkt.seq );
  sample_to_le(pkt.samples);
  sample_to_le(pkt.samples + 3);
  sample_to_le(pkt.samples + 6);
  sample_to_le(pkt.samples + 9);
  return pkt;
}

static void write_json(packet_t pkt){
  static uint8_t first = 1;
  char buff[2048] = {0};
  uint32_t to_write, written = 0, status;
  uint32_t sample1 = 0, sample2 = 0, sample3 = 0, sample4 = 0;  

  pkt = ntohpkt(pkt);

  if(!first)
    write(json_fd, ",\n", 2);
  else{
    write(json_fd, "\n", 1);
    first = 0;
  }

  memcpy(&sample1, pkt.samples, 3);
  memcpy(&sample2, pkt.samples + 3, 3);
  memcpy(&sample3, pkt.samples + 6, 3);
  memcpy(&sample4, pkt.samples + 9, 3);

#ifdef __GPS__
  if(test){
    struct timeval t;
    gettimeofday(&t, NULL);

    pkt.timestamp = (((int64_t) t.tv_sec) * 1000000 + t.tv_usec) - pkt.timestamp;
    pkt.gps_us = get_millis_offset() - pkt.gps_us;
  }
  sprintf(buff, "\"%u:%u\" : {\"gps_us\" : %lld, \"timestamp\" : %lld, \"air_time\" : %lld, \"sequence\" : %u, \"fails\" : %u, \"retries\" : %u, \"sample_1\" : %u, \"sample_2\" : %u, \"sample_3\" : %u, \"sample_4\" : %u \"node_id\" : %u }", pkt.id, pkt.seq, pkt.gps_us, pkt.timestamp, pkt.air, pkt.seq, pkt.fails, pkt.retries, sample1, sample2, sample3, sample4, pkt.id);
#else
  sprintf(buff, "\"%u:%u\" : {\"timestamp\" : %lld, \"air_time\" : %lld, \"sequence\" : %u, \"fails\" : %u, \"retries\" : %u, \"sample_1\" : %u, \"sample_2\" : %u, \"sample_3\" : %u, \"sample_4\" : %u \"node_id\" : %u }", pkt.id, pkt.seq, pkt.timestamp, pkt.air, pkt.seq, pkt.fails, pkt.retries, sample1, sample2, sample3, sample4, pkt.id);
#endif
  to_write = strlen(buff);
  while(written < to_write){
    status = write(json_fd, buff + written, to_write - written);
    if(status == -1){
      perror("Unable to write to json file");
      return;
    }
    written += status;
  }
}

static uint8_t open_output_files(){
  bin_fd = open(output_binary_file, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if(bin_fd == -1){
    perror("Unable to open binary output file");
    return 0;
  }

  json_fd = open(output_json_file, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if(json_fd == -1){
    close(bin_fd);
    perror("Unable to open json output file");
    return 0;
  }
  write(json_fd, "{", 1);
  return 1;
}

static void close_output_files(){
  close(bin_fd);
  write(json_fd, "\n}", 2);
  close(json_fd);
}

static void dump(list* l){
  char buff[100] = {0};
  uint32_t i;

  sprintf(buff, "%s.bin", l->new_filename);
  rename(output_binary_file, buff);

  memset(buff, 0, sizeof(buff));
  sprintf(buff, "%s.json", l->new_filename);
  rename(output_json_file, buff);
  
  if(open_output_files()){
    for(i = 0; i < l->lst_size; i++){
      write_bin(l->buff[i]);
      write_json(l->buff[i]);
    }
    close_output_files();
  }
}

//Because timestamp is int64_t and this should return int we cannot just do a.timestamp - b.timestamp.
static int  __packet_comparator(const void* a, const void* b){
  int64_t at = (*((packet_t*) a)).timestamp;
  int64_t bt = (*((packet_t*) b)).timestamp;

  if(at < bt)
    return -1;
  if(at > bt)
    return 1;
  return 0;
}

static void rotate(list* l){
  qsort(l->buff, l->lst_size, sizeof(packet_t), __packet_comparator);
  dump(l);
  l->lst_size = 0;
  memset(l->buff, 0, sizeof(l->rotate_at));
}

void insert(list* l, packet_t* p){

  memcpy(l->buff + l->lst_size, p, sizeof(*p));

  l->lst_size++;
  if(l->rotate_at == l->lst_size)
    rotate(l);
}
