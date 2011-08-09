/*
 * =====================================================================================
 *
 *       Filename:  simple.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/08/2011 07:26:05 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  
 *
 * =====================================================================================
 */

#include <assert.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sched.h>
#include <stdlib.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/mman.h>

int * cvspiregs;
int * cvgpioregs;

void *map_phys(off_t addr,int *fd) {
    off_t page;
    unsigned char *start;

    start = mmap(0, getpagesize(), PROT_READ|PROT_WRITE, MAP_SHARED, *fd, page);
    start = start + (addr & 0xfff);

    return start;
}

void _init_cavium() {
  int i;

  cvspiregs[0x64 / 4] = 0x0; /* RX IRQ threshold 0 */
  cvspiregs[0x40 / 4] = 0x80000c02; /* 24-bit mode, no byte swap */
  cvspiregs[0x60 / 4] = 0x0; /* 0 clock inter-transfer delay */
  cvspiregs[0x6c / 4] = 0x0; /* disable interrupts */
  cvspiregs[0x4c / 4] = 0x4; /* deassert CS# */
  for (i = 0; i < 8; i++) cvspiregs[0x58 / 4];
  cvgpioregs[0] = (2<<15|1<<17|1<<3);
  cavium_spi_speed(last_freq,last_edge);
  cavium_disable_cs(); // force CS# deassertion just in case
}

int main(void){
    int *fd;
    int devmem = -1;

    *fd = -1;
    *fd = open("/dev/mem", O_RDWR|O_SYNC);
    if (*fd == -1) {
        perror("open(/dev/mem):");
        return 0;
    }

    cvspiregs = map_phys(0x71000000,&devmem);
    cvgpioregs = map_phys(0x7c000000,&devmem);

    cvspiregs[0x40 / 4] = (CLK_14|SPEED_13_10|LUN_9_8)

    read = cvspiregs[0x4a / 4]
    read = cvspiregs[0x4c / 4]
}

