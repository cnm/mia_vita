#include <stdio.h>
#include <string.h>

void print_usage(char* cmd){
  printf("This program will just receive an error code as argument and print the error message associate with it.\nThis is useful because I don't have this ease in kernel land and can only print the error code.\n");
  printf("\nUsage: %s <error_code>\n", cmd);
}

int print_error(char* cmd, char* error){
  int i;
  for(i = 0; error[i] != '\0'; i++)
    if(!isdigit(error[i])){
      print_usage(cmd);
      return -1;
    }

  printf("%s\n", strerror(atoi(error)));
  return 0;
}

int main(int argc, char** argv){
  if(argc == 1){
    print_usage(argv[0]);
    return -1;
  }

  return print_error(argv[0], argv[1]);
}
