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

#define SPI_MEM_ADDRESS     0x71000000
#define GPIO_MEM_ADDRESS    0x7C000000

#define LUN_CS_ERASE_MASK   (0x700)      /* The bit 8 and 9 is 1 the rest is 0 */
#define LUN_CS_BIT_MASK     (1<<8)      /* The bit 9 is 1 the rest is 0 */
#define SPEED_MASK          (7<<10)     /* Mask to reset the speed          */
#define EDGE_MASK           (1<<14)     /* The edge is set in the 14 bit    */
#define SPEED_SHIFT         10          /* 10 bits to 13 set the speed      */

#define MOD 1

#ifndef MOD
#include <stdio.h> /* For printf */
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h> /* For getpagesize  */

#define print printf

void *map_phys(off_t addr) {
    off_t page;
    unsigned char *start;
    int fd;

    fd = open("/dev/mem", O_RDWR|O_SYNC);
    page = addr & 0xfffff000;
    start = mmap(0, getpagesize(), PROT_READ|PROT_WRITE, MAP_SHARED, fd, page);
    if (start == MAP_FAILED) {
        print("Error");
        return 0;
    }

    start = start + (addr & 0xfff);
    print("Start %p\n", start);
    return start;
}


#else

#include <linux/module.h>
#include <linux/kernel.h>       /* printk() */

#define print printk

extern int request_mem(volatile unsigned int mem_addr, unsigned int size);
extern void release_mem(volatile unsigned int mem_addr, unsigned int byte_size);
void release_mem_spi(void);

void *map_phys(int addr) {
    if(addr == GPIO_MEM_ADDRESS){
        return (void*) request_mem(GPIO_MEM_ADDRESS, 4);
    }

    else if(addr == SPI_MEM_ADDRESS){
        return (void*) request_mem(SPI_MEM_ADDRESS, 0x6c);
    }
    else{
        print("Error\n");
        return NULL;
    }
}


#endif



static volatile unsigned int * spi_new_mem, *gpio_new_mem;
void set_lun_speed_edge(void);
unsigned short getR0(void);
void cavium_disable_cs(void);
int dostuff(void);
void prepare_registers(void);
void release_mem_spi(void);

void cavium_poke16(unsigned int adr, unsigned short dat) {
    unsigned int dummy = -1;
    unsigned int d = dat;
    print("POKE Start %p\n", spi_new_mem);

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
                  : "+r"(dummy) 
                  : "r"(adr), "r"(d), "r"(spi_new_mem) 
                  : "r1","cc"
    );

    print("\tPOKE16 dat=%04X,adr=%04X\n",dat,adr);
}

unsigned short cavium_peek16(unsigned int adr) {
    unsigned short ret = -1;
    print("PEEK Start %p\n", spi_new_mem);

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
                  : "+r"(ret) 
                  : "r"(adr), "r"(spi_new_mem) 
                  : "r1", "cc"
    );
    print("\tPEEK16 dat=%04X,adr=%04X,\n",ret,adr);
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
        print("cavium_disable_cs:%04X\n",val);
        setR0(val & ~(1<<7));
        setR0((val & ~(1<<7)) ^ (1 << 14));
    }
    else {
        print("cavium_disable_cs:%04X (NOP)\n",val);
    }
}

void prepare_registers() {
    int i;

    spi_new_mem[0x64 / 4] = 0x0;        /* RX IRQ threshold 0 */
    spi_new_mem[0x40 / 4] = 0x80000c02; /* 24-bit mode, no byte swap */
    spi_new_mem[0x60 / 4] = 0x0;        /* 0 clock inter-transfer delay */
    spi_new_mem[0x6c / 4] = 0x0;        /* disable interrupts */
    spi_new_mem[0x4c / 4] = 0x4;        /* deassert CS# */

    for (i = 0; i < 8; i++) spi_new_mem[0x58 / 4];

    gpio_new_mem[0] = (2<<15|1<<17|1<<3);

    cavium_disable_cs(); // force CS# deassertion just in case
}


void reserve_memory(void){
    spi_new_mem  = map_phys(SPI_MEM_ADDRESS);
    gpio_new_mem = map_phys(GPIO_MEM_ADDRESS);
}

void prepare(void){
    reserve_memory();
    print("Preparing registers\n");
    prepare_registers();
    print("Ended Preparing registers\n");


    print("Setting parameters\n");
    set_lun_speed_edge();
    print("End setting parameters\n");
}


void set_lun_speed_edge(){
    int clock = 15;
    int edge = 1;

    unsigned int mask = 0;
    unsigned int conf = getR0();

    /* Set the edge */
    if (edge == 1)
      mask |= EDGE_MASK;
    else if (edge == 0)
      mask &= ~EDGE_MASK;
    print("Writing the edge\n");
    setR0(conf | mask);

    /* Set the lun  */
    mask |= LUN_CS_BIT_MASK;
    conf &= ~LUN_CS_ERASE_MASK;

    /* Set the speed  */
    mask |= clock << SPEED_SHIFT;

    print("Writing the speed and lun\n");
    setR0(conf | mask);


    print("MASCARA %X\n", mask);
    print("CONF: %X\n", conf);
}

unsigned int read_32_bits(void){
    unsigned int ret;
    unsigned short l, h;

    l = cavium_peek16(0x0A);
    h = cavium_peek16(0x8);

    ret = (l|(h<<16));

    return ret;
}

#ifndef MOD
int main(int argc, char **argv) {
    int result = 0;

    /* Prepare ???  */
    prepare();

    /* Execute things  */
    result = read_32_bits();

    /* Print results  */
    print("Resultado %u\n", result);

    print("\n");

    return 0;
}
#else
int dostuff() {
    int result = 0;

    printk("Doing stuff");

    /* Prepare ???  */
    prepare();

    /* Execute things  */
    result = read_32_bits();

    /* Print results  */
    print("Resultado %u\n", result);
    print("\n");

    return 0;
}

void release_mem_spi(){
    release_mem(GPIO_MEM_ADDRESS, 4);
    release_mem(SPI_MEM_ADDRESS, 0x6c);
}
#endif
