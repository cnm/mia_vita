#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <miavita_packet.h>
#include <arpa/inet.h>
#include <string.h>
#include <byteswap.h>

#define PORT 57844

packet_t create_fake_packet(uint32_t seq_number, char * number);

void send_packet(const packet_t *);
void bind_port(char * server_ip);

struct sockaddr_in serv_addr;
int sockfd, i, slen=sizeof(serv_addr);

int main(int argc, char** argv)
{
  uint32_t seq = 0;

  if(argc != 3)
    {
      printf("Usage : %s <Server-IP> <Node-Number>\n",argv[0]);
      exit(1);
    }

  bind_port(argv[1]);


  while(1){
    packet_t p = create_fake_packet(seq++, argv[2]);
    send_packet(&p);
    usleep(13); //1000 microsecond= 1 second will sleep...
  }

  return 0;
}

void send_packet(const packet_t * p ) {
    size_t length = sizeof(packet_t);
    if (sendto(sockfd, p, length, 0, (struct sockaddr*) &serv_addr, slen)==-1){
     perror("sendto()");
    }
}

void bind_port(char * server_ip) {

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1) {
      perror("socket");
      exit(1);
    }

    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    if (inet_aton(server_ip, &serv_addr.sin_addr)==0)
      {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
      }
}

packet_t create_fake_packet(uint32_t seq_number, char * number) {
    packet_t p;
    p.id = atoi(number);
    p.seq = bswap_32(seq_number);
    p.timestamp = bswap_64(seq_number);
    return p;
}

