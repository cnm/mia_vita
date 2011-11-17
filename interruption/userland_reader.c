#include <stdio.h>
#include <time.h>

void main()
{
  int i;
  FILE *ifp;
  int getc= 0;

  ifp = fopen("/proc/geophone", "r");

  while (i != EOF) {
      i = getc(ifp);
      printf("Reading");

      
      printf(" %c\n",i);
      nc++;
  }   
  printf("OuT\n");
}
