#include <stdio.h>
#include <time.h>

void main()
{
  int i, nc;
  FILE *ifp;

  ifp = fopen("/proc/geophone", "r");

  i = getc(ifp);
  while (1) {
      printf("Reading");
      i = getc(ifp);
      printf("%c\n",i);
  }   
  printf("OuT\n");
}
