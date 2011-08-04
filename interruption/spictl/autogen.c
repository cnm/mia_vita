#include <stdio.h>

int main() {
  int i,j,k,n,val;
  char buf[8];

  for (i=0;i<6561;i++) { // 3^8
    n = i;
    val = 0;
    for (j=0;j<8;j++) {
      k = n % 3;
      switch (k) {
      case 0: buf[7-j] = '0'; break;
      case 1: buf[7-j] = '1'; val = val + 0x80; break;
      case 2: buf[7-j] = 'x'; break;
      default: abort();
      }
      n = n / 3;
      if (j < 7) val >>= 1;
    }
    printf("#define b");
    for (j=0;j<4;j++) putc(buf[j],stdout);
    printf("_");
    for (j=4;j<8;j++) putc(buf[j],stdout);
    printf(" 0x%02X\n",val);
  }
  //#define b00xx_xxxx 0x00
}
