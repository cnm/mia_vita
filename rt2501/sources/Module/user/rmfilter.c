#include <stdio.h>
#include <string.h>
#include "proc_write.h"

int main(int argc, char** argv){
  if(argc != 2){
    printf("Usage: %s <id>\n", argv[0]);
    return 1;
  }

  write_to_proc_entry('U', 0, 0, 0, 0, atoi(argv[1]));
  return 0;
}
