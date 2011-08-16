/*
 * =====================================================================================
 *
 *       Filename:  caller.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/16/2011 12:40:08 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  
 *
 * =====================================================================================
 */

#include <stdio.h> /* For printf */

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h> /* For getpagesize  */


#define SPI_MEM_ADDRESS     0x71000000
#define MISC_MEM_ADDRESS    0x76000000

#define LUN_CS_ERASE_MASK   (0x700)      /* The bit 8 and 9 is 1 the rest is 0 */
#define LUN_CS_BIT_MASK     (1<<8)      /* The bit 9 is 1 the rest is 0 */
#define SPEED_MASK          (3<<10)     /* Mask to reset the speed          */
#define SPEED_SHIFT         10          /* 10 bits to 13 set the speed      */

#define EDGE_MASK           (1<<14)     /* The edge is set in the 14 bit    */



static volatile unsigned int * spi_new_mem, *gpio_new_mem;
void set_lun_speed_edge(void);

void cavium_poke16(unsigned int adr, unsigned short dat) {
    unsigned int dummy = -1;
    unsigned int d = dat;

    printf("\tPOKE16 dat=%04X,adr=%04X\n",dat,adr);
    asm volatile (
                  "mov %0, %1, lsl #18\n"
                  "orr %0, %0, #0x800000\n"
                  "orr %0, %0, %2, lsl #3\n"
                  "3: ldr r1, [%3, #0x64]\n"
                  "cmp r1, #0x0\n"
                  "bne 3b\n"
                  "2: str %0, [%3, #0x50]\n"
                  "1: ldr r1, [%3, #0x64]\n"
                  "cmp r1, #0x0\n"
                  "beq 1b\n"
                  "ldr %0, [%3, #0x58]\n"
                  "ands r1, %0, #0x1\n"
                  "moveq %0, #0x0\n"
                  "beq 3b\n"
                  : "+r"(dummy) : "r"(adr), "r"(d), "r"(spi_new_mem) : "r1","cc"
    );
}

unsigned short cavium_peek16(unsigned int adr) {
    unsigned short ret = -1;

    asm volatile (
                  "mov %0, %1, lsl #18\n"
                  "2: str %0, [%2, #0x50]\n"
                  "1: ldr r1, [%2, #0x64]\n"
                  "cmp r1, #0x0\n"
                  "beq 1b\n"
                  "ldr %0, [%2, #0x58]\n"
                  "ands r1, %0, #0x10000\n"
                  "bicne %0, %0, #0xff0000\n"
                  "moveq %0, #0x0\n"
                  "beq 2b\n" 
                  : "+r"(ret) : "r"(adr), "r"(spi_new_mem) : "r1", "cc"
    );
    printf("\tPEEK16 dat=%04X,adr=%04X,\n",ret,adr);
    return ret;
}

unsigned short getR0() {
  unsigned short spiR0 = cavium_peek16(0);
  return spiR0;
}

void setR0(unsigned short val) {
  cavium_poke16(0,val);
}


void cavium_disable_cs() {
    unsigned short val = getR0();

    if (val & (1<<7)) {
        printf("cavium_disable_cs:%04X\n",val);
        setR0(val & ~(1<<7));
        setR0((val & ~(1<<7)) ^ (1 << 14));
    }
    else {
        printf("cavium_disable_cs:%04X (NOP)\n",val);
    }
}

void prepare_registers() {
    int i;

    spi_new_mem[0x64 / 4] = 0x0;        /* RX IRQ threahold 0 */
    spi_new_mem[0x40 / 4] = 0x80000c02; /* 24-bit mode, no byte swap */
    spi_new_mem[0x60 / 4] = 0x0;        /* 0 clock inter-transfer delay */
    spi_new_mem[0x6c / 4] = 0x0;        /* disable interrupts */
    spi_new_mem[0x4c / 4] = 0x4;        /* deassert CS# */

    for (i = 0; i < 8; i++) spi_new_mem[0x58 / 4];

    gpio_new_mem[0] = (2<<15|1<<17|1<<3);

    cavium_disable_cs(); // force CS# deassertion just in case
}

void *map_phys(off_t addr) {
    off_t page;
    unsigned char *start;
    int fd;

    fd = open("/dev/mem", O_RDWR|O_SYNC);
    page = addr & 0xfffff000;
    start = mmap(0, getpagesize(), PROT_READ|PROT_WRITE, MAP_SHARED, fd, page);
    if (start == MAP_FAILED) {
        printf("Error");
        return 0;
    }

    start = start + (addr & 0xfff);
    return start;
}


void reserve_memory(void){
    spi_new_mem  = map_phys(SPI_MEM_ADDRESS);
    gpio_new_mem = map_phys(MISC_MEM_ADDRESS);
}

void prepare(void){
    reserve_memory();
    printf("Preparing registers\n");
    prepare_registers();
    printf("Ended Preparing registers\n");


    printf("Setting speed\n");
    set_lun_speed_edge();
    printf("End setting speed\n");
}


void set_lun_speed_edge(){
    int clock = 1;
    int edge = 1;

    unsigned int mask = 0;
    unsigned int conf = getR0(0);

    /* Set the edge */
    if (edge == 1)
      mask |= EDGE_MASK;
    else if (edge == 0)
      mask &= ~EDGE_MASK;
    setR0(conf | mask);

    /* Set the lun  */
    mask |= LUN_CS_BIT_MASK;
    conf &= ~LUN_CS_ERASE_MASK;

    /* Set the speed  */
    mask |= clock << SPEED_SHIFT;

    setR0(conf | mask);
}

int main(int argc, char **argv) {

    /* Prepare ???  */
    prepare();

    /* Execute things  */
/*    process_options(argc,argv,opts);*/
/*    rbuf = spi_execute(&bytes);*/


    /* Print results  */
/*    printf("#Read bytes:\n");*/
/*    print_octal(rbuf, (unsigned int) bytes);*/
/*    print_int(rbuf);*/
/*    printf("Teste: %u\n", result);*/

    printf("\n");

    return 0;
}

