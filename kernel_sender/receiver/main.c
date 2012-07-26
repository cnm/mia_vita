#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <signal.h>
#include <unistd.h>


#ifdef __GPS__
#include "syscall_wrapper.h"
#endif

#include "miavita_packet.h"
#include "list.h"

uint16_t port = 57844;
char* iface = "lo", *move_file_to = "miavita_old";
uint32_t capacity = 100;

int sockfd = -1;

list *l;

void print_usage(char* cmd)
{
  printf("Usage: %s [-i <interface>] [-p <listen_on_port>] [-b <output_binary_file>] [-j <output_json_file>] [-o <moved_file_prefix>]\n", cmd);
  printf("-i\tInterface name on which the program will listen. Default is %s\n", iface);
  printf("-p\tUDP port on which the program will listen. Default is %u\n", port);
  printf("-b\tName of the binary file to where the data is going to be written. Default is %s\n", output_binary_file);
  printf("-j\tName of the json file to where the data is going to be written. Default is %s\n", output_json_file);
  printf("-t\tTest the program against GPS time. Make sure to compile this program with -D__GPS__.\n");
  printf("-o\tOutput file prefix when the file is moved by log rotation. Default is %s.\n", move_file_to);
  printf("-c\tBuffer capacity expressed in terms of number of packets. Default is %d.\n", capacity);
}

uint8_t parse_args(char** argv, int argc)
{
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
      if(!strcmp(argv[i], "-c")){
          capacity = atoi(argv[i + 1]);
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
      if(!strcmp(argv[i], "-o")){
          move_file_to = argv[i + 1];
          i += 2;
          continue;
      }
      print_usage(argv[0]);
      return 0;
  }
  return 1;
}

uint8_t bind_socket()
{
  struct ifaddrs *addrs, *iap;
  struct sockaddr_in sa;

  sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if(sockfd == -1){
      perror("Unable to create socket");
      return 0;
  }

  getifaddrs(&addrs);
  for (iap = addrs; iap != NULL; iap = iap->ifa_next)
    {
      if (iap->ifa_addr && (iap->ifa_flags & IFF_UP) && iap->ifa_addr->sa_family == AF_INET)
        {
          if (!strcmp(iap->ifa_name, iface))
            {
              memcpy(&sa, (struct sockaddr_in *)(iap->ifa_addr), sizeof(sa));
              sa.sin_port = htons(port);
              if(bind(sockfd, (struct sockaddr*) &sa, sizeof(struct sockaddr)) == -1)
                {
                  close(sockfd);
                  perror("Unable to bind socket to addrs:");
                  return 0;
                }
              printf("Binded socket to interface: %s on port %d\n", iface, port);
              return 1;
            }
        }
    }

#ifdef __DEBUG__
  printf("Connected to socket\n");
#endif
  return 1;
}


/* Binds sockets, receives packets and inserts them in list l*/
void serve()
{

  l = mklist(capacity, move_file_to);

  while(1)
    {
      struct sockaddr_in sa;
      packet_t pkt;
      uint32_t size = sizeof(sa);

      memset(&sa, 0, sizeof(sa));
      memset(&pkt, 0, sizeof(pkt));

#ifdef __DEBUG__
      printf("Going to receive packets (it is blocking)\n");
#endif
      if(recvfrom(sockfd, (void*) &pkt, sizeof(pkt), 0, (struct sockaddr*) &sa, &size) <= 0){
          perror("recvfrom() - Received from the socket gave an error");
          return;
      }
#ifdef __DEBUG__
      printf("Received packet.\n");
#endif
      insert(l, &pkt);
    }
}

void cleanup()
{
  close(sockfd);
  rmlist(l);
  exit(0);
}

int main(int argc, char** argv)
{
  if(!parse_args(argv, argc) || !bind_socket())
    {
      printf("Some error ocurred. Leaving the program\n");
      return 1;
    }

  signal(SIGINT, cleanup);
  serve();
  return 0;
}
