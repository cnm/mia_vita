#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include "proc_write.h"

int main(int argc, char** argv){
  int i;
  if(argc == 1){
    printf("Please specify at least one parameter.\n");
    printf("Usage: %s -da <dst_ip> -sa <src_ip> -dp <dst_port> -sp <src_port>\n", argv[0]);
    return 1;
  }


  uint32_t daddr = 0, saddr = 0;
  uint16_t dport = 0, sport = 0;

  for(i = 1; i < argc;){
    if(!strcmp(argv[i], "-da")){
      if (inet_pton(AF_INET, argv[i + 1], &daddr) < 0) {
	printf("Could not convert address %s\n", argv[i + 1]);
	return 1;
      }
      i += 2;
      continue;
    }
    if(!strcmp(argv[i], "-sa")){
      if (inet_pton(AF_INET, argv[i + 1], &saddr) < 0) {
	printf("Could not convert address %s\n", argv[i + 1]);
	return 1;
      }
      i += 2;
      continue;
    }
    if(!strcmp(argv[i], "-dp")){
      dport = htons(atoi(argv[i + 1]));
      i += 2;
      continue;
    }
    if(!strcmp(argv[i], "-sp")){
      sport = htons(atoi(argv[i + 1]));
      i += 2;
      continue;
    }
    i++;
  }

  write_to_proc_entry('R', daddr, saddr, sport, dport, 0);
  return 0;
}
