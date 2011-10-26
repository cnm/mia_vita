#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char* argv[]) {

    char c[4]; int n;
    unsigned int value;

    while(n = read(0, &c[0], 4)){
        if(n!=4){
            printf("Error %d\n",n);
            return 1;
        }

/*        printf("READ: %02X %02X %02X %02X\n", c[0], c[1], c[2], c[4]);*/

        value = c[0] | c[1] << 8 | c[2] << 16 ;
/*       printf("PIPE: %02X\n", value, n);*/
       printf("%u\n", value);
    }

}
