#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <endian.h>
#include <signal.h>

#include "miavita_packet.h"

uint16_t port = 57843;
char* iface = "eth0";
char* output_binary_file = "miavita.bin";
char* output_json_file = "miavita.json";

int sockfd = -1, bin_fd = -1, json_fd = -1;

void print_usage(char* cmd){
  printf("Usage: %s [-i <interface>] [-p <listen_on_port>] [-b <output_binary_file>] [-j <output_json_file>]\n", cmd);
  printf("-i\tInterface name on which the program will listen. Default is %s\n", iface);
  printf("-p\tUDP port on which the program will listen. Default is %u\n", port);
  printf("-b\tName of the binary file to where the data is going to be written. Default is %s\n", output_binary_file);
  printf("-j\tName of the json file to where the data is going to be written. Default is %s\n", output_json_file);
}

uint8_t parse_args(char** argv, int argc){
  uint32_t i;
  for(i = 1; i < argc;){
    if(!strcmp(argv[i], "-i")){
      iface = argv[i + 1];
      i += 2;
      continue;
    }
    if(!strcmp(argv[i], "-p")){
      port = atoi(argv[i + 1]);
      i += 2;
      continue;
    }
    if(!strcmp(argv[i], "-b")){
      output_binary_file = argv[i + 1];
      i += 2;
      continue;
    }
    if(!strcmp(argv[i], "-j")){
      output_json_file = argv[i + 1];
      i += 2;
      continue;
    }
    print_usage(argv[0]);
    return 0;
  }
  return 1;
}

uint8_t bind_socket() {
  struct ifaddrs *addrs, *iap;
  struct sockaddr_in sa;
  char buf[32];

  sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if(sockfd == -1){
    perror("Unable to create socket");
    return 0;
  }

  getifaddrs(&addrs);
  for (iap = addrs; iap != NULL; iap = iap->ifa_next) {
    if (iap->ifa_addr && (iap->ifa_flags & IFF_UP) && iap->ifa_addr->sa_family == AF_INET) {
      if (!strcmp(iap->ifa_name, iface)) {
	memcpy(&sa, (struct sockaddr_in *)(iap->ifa_addr), sizeof(sa));
	sa.sin_port = htons(port);
	if(bind(sockfd, (struct sockaddr*) &sa, sizeof(struct sockaddr)) == -1){
	  close(sockfd);
	  perror("Unable to bind socket to interface");
	  return 0;
	}
	return 1;
      }
    }
  }

  return 1;
}

void write_bin(packet_t pkt){
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

packet_t ntohpkt(packet_t pkt){
  pkt.timestamp = be64toh( pkt.timestamp );
  pkt.air = be64toh( pkt.air );
  pkt.seq = be32toh( pkt.seq );
  sample_to_le(pkt.samples);
  sample_to_le(pkt.samples + 3);
  sample_to_le(pkt.samples + 6);
  sample_to_le(pkt.samples + 9);
  return pkt;
}

void write_json(packet_t pkt){
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

  sprintf(buff, "\"%u:%u\" : {\"timestamp\" : %lld, \"air_time\" : %lld, \"sequence\" : %u, \"fails\" : %u, \"retries\" : %u, \"sample_1\" : %u, \"sample_2\" : %u, \"sample_3\" : %u, \"sample_4\" : %u \"node_id\" : %u }", pkt.id, pkt.seq, pkt.timestamp, pkt.air, pkt.seq, pkt.fails, pkt.retries, sample1, sample2, sample3, sample4, pkt.id);

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

void serve(){
  while(1){
    struct sockaddr_in sa;
    packet_t pkt;
    uint32_t size = sizeof(sa);

    memset(&sa, 0, sizeof(sa));
    memset(&pkt, 0, sizeof(pkt));

    if(recvfrom(sockfd, (void*) &pkt, sizeof(pkt), 0, (struct sockaddr*) &sa, &size) <= 0){
      perror("recvfrom()");
      return;
    }

    write_bin(pkt);
    write_json(pkt);
  }
}

uint8_t open_output_files(){
  bin_fd = open(output_binary_file, O_WRONLY | O_TRUNC | O_CREAT);
  if(bin_fd == -1){
    perror("Unable to open binary output file");
    return 0;
  }

  json_fd = open(output_json_file, O_WRONLY | O_TRUNC | O_CREAT);
  if(json_fd == -1){
    close(bin_fd);
    perror("Unable to open json output file");
    return 0;
  }
  write(json_fd, "{", 1);
  return 1;
}

void cleanup(){
  close(bin_fd);
  write(json_fd, "\n}", 2);
  close(json_fd);
  close(sockfd);
  exit(0);
}

int main(int argc, char** argv){
  if(!parse_args(argv, argc) || !bind_socket() || !open_output_files())
    return 1;
  signal(SIGINT, cleanup);
  serve();
  return 0;
}
