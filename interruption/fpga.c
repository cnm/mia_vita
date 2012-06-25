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

#include <linux/module.h>
#include <linux/kernel.h>       /* printk() */

#ifdef __GPS__
#include <linux/miavita_xtime.h>
#endif

#include "mem_addr.h"

extern int request_mem(volatile unsigned int mem_addr, unsigned int size);
extern void release_mem(volatile unsigned int mem_addr, unsigned int byte_size);
void write_dio26(bool b);
unsigned short read_dio26(void);
void release_mem_spi(void);

void set_lun_speed_edge(void);
unsigned short getR0(void);
void cavium_disable_cs(void);
int dostuff(void);
void prepare_registers(void);
static volatile unsigned int gpio_a_new_mem;
static volatile unsigned int spi_register;

static volatile unsigned int *cvspiregs;

void write_watchdog(void);

void poke16(unsigned int adr, unsigned short dat) {
    unsigned int dummy = 0;
    unsigned int d = dat;
    unsigned int a15_a16_addr_register; /*  See section 5.1 of the ts7500 manual */
    volatile unsigned int *p; // The volatile is extremely important here

    a15_a16_addr_register = (adr>>5) << 15;
    p = (unsigned int *) gpio_a_new_mem;
    *p = (a15_a16_addr_register|1<<17|1<<3); /* Enable I2SWS, I2SCLK and I2SDR */

    adr &= 0x1f;

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
                  : "r"(adr), "r"(d), "r"(cvspiregs)
                  : "r1","cc"
    );

}

unsigned short peek16(unsigned int adr) {
    unsigned short ret = -1;
    unsigned int a15_a16_addr_register; /*  See section 5.1 of the ts7500 manual */
    volatile unsigned int *p; // The volatile is extremely important here

    a15_a16_addr_register = (adr>>5) << 15;
    p = (unsigned int *) gpio_a_new_mem;
    *p = (a15_a16_addr_register|1<<17|1<<3); /* Enable I2SWS, I2SCLK and I2SDR */

    adr &= 0x1f;

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
                  : "r"(adr), "r"(cvspiregs)
                  : "r1", "cc"
    );

    return ret;
}

unsigned short getR0() {
    unsigned short spiR0 = peek16(0x40);
    return spiR0;
}

void setR0(unsigned short val) {
    poke16(0x40,val);
}

void cavium_disable_cs() {
    unsigned short val = getR0();

    if (val & (1<<7)) {
        /*        printk("cavium_disable_cs:%04X\n",val);*/
        setR0(val & ~(1<<7));
        setR0((val & ~(1<<7)) ^ (1 << 14));
    }
    else {
        /*        printk("cavium_disable_cs:%04X (NOP)\n",val);*/
    }
}

void prepare_registers() {
    int i;
    volatile unsigned int *p; // The volatile is extremely important here

    p = (unsigned int *) (spi_register + SPI_FIFO_RX_CFG);
    *p = 0x0;        /* RX IRQ threshold 0 */

    p = (unsigned int *) (spi_register + SPI_CFG);
    *p = 0x80000c02; /* 24-bit mode, no byte swap */

    p = (unsigned int *) (spi_register + SPI_FIFO_TX_CTRL);
    *p  = 0x0;        /* 0 clock inter-transfer delay */

    p = (unsigned int *) (spi_register + SPI_INTR_ENA);
    *p = 0x0;        /* disable interrupts */

    p = (unsigned int *) (spi_register + SPI_TX_CTRL);
    *p = 0x4;        /* deassert CS# */

    p = (unsigned int *) (spi_register + SPI_RX_DATA); //Read SPI BUFFER (to cleanup)
    for (i = 0; i < 8; i++) *p;

    p = (unsigned int *) gpio_a_new_mem;
    *p = (2<<15|1<<17|1<<3); /* Enable I2SWS, I2SCLK and I2SDR */

    cavium_disable_cs(); // force CS# deassertion just in case
}

void reserve_memory(void){
    gpio_a_new_mem = request_mem(GPIOA_REGISTER, WORD_SIZE);
    spi_register = request_mem(SPI_REGISTER, 0x6C + WORD_SIZE);
    cvspiregs = (void*) spi_register;
}

void prepare_spi2(void){
    printk("Preparing registers\n");
    prepare_registers();
    printk("Ended Preparing registers\n");

    printk("Setting parameters\n");
    set_lun_speed_edge();
    printk("End setting parameters\n");
}

void prepare_spi(void){
    printk("Reserving memory\n");
    reserve_memory();

}

void set_lun_speed_edge(){
    int clock = 15;
    int edge = 1; /* It must reads on the rising edge */

    unsigned int mask = 0;
    unsigned int conf = getR0();

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

    /*    printk("Writing the speed and lun\n");*/
    setR0(conf | mask);
}

#define SEC_2_NSEC 1000000000L
#define USEC_2_NSEC 1000

int64_t get_kernel_current_time(void) {
  struct timeval t;
  memset(&t, 0, sizeof(struct timeval));
  do_gettimeofday(&t);
  return ((int64_t) t.tv_sec) * SEC_2_NSEC + ((int64_t) t.tv_usec)
    * USEC_2_NSEC;
}

#ifdef __GPS__
void read_four_channels(unsigned int* read_buffer, int64_t* timestamp, int64_t* gps_us){
    unsigned int a,b,c,d,e,f;
    a = b = c = d = e = f = 0;

    *timestamp = get_kernel_current_time();
    *gps_us = __miavita_elapsed_secs * 1000000 + __miavita_elapsed_usecs;

    a = peek16(0x4A);//2/3 da primeira
    b = peek16(0x4A);//1/3 da primeira 1/3 da segunda
    c = peek16(0x4A);//2/3 da segunda
    d = peek16(0x4A);//2/3 da terceira
    e = peek16(0x4A);//1/3 da terceira 1/3 da quarta

    f = peek16(0x4C);//2/3 da quarta

    read_buffer[0] = (a<<16|b);
    read_buffer[1] = (c<<16|d);
    read_buffer[2] = (e<<16|f);


    /* Usefull to debug stuff */
/*    read_buffer[0] = 0x11223344;*/
/*    read_buffer[1] = 0x55667788;*/
/*    read_buffer[2] = 0x99AABBCC;*/

/*    printk(KERN_INFO "DATA: %x - %x - %x - %x - %x - %x \n", a, b, c, d, e, f);*/

    return;
}
#else
void read_four_channels(unsigned int* read_buffer, int64_t* timestamp){
    unsigned int a,b,c,d,e,f;
    a = b = c = d = e = f = 0;

    *timestamp = get_kernel_current_time();

    a = peek16(0x4A);//2/3 da primeira
    b = peek16(0x4A);//1/3 da primeira 1/3 da segunda
    c = peek16(0x4A);//2/3 da segunda
    d = peek16(0x4A);//2/3 da terceira 
    e = peek16(0x4A);//1/3 da terceira 1/3 da quarta

    f = peek16(0x4C);//2/3 da quarta

    read_buffer[0] = (a<<16|b);
    read_buffer[1] = (c<<16|d);
    read_buffer[2] = (e<<16|f);

    /* Usefull to debug stuff */
/*    read_buffer[0] = 0x11223344;*/
/*    read_buffer[1] = 0x55667788;*/
/*    read_buffer[2] = 0x99AABBCC;*/

/*    printk(KERN_EMERG "DATA: %04X - %04X - %04X - %04X - %04X - %04X \n", a, b, c, d, e, f);*/

    return;
}
#endif

void release_mem_spi(void){
    release_mem(GPIOA_REGISTER, WORD_SIZE);
    release_mem(SPI_REGISTER, 0x6C + WORD_SIZE);
}

void write_dio26(bool b){
    int pinOffSet = 5;

    if(b){
        poke16(0x6a, (peek16(0x6a) | (1 << pinOffSet)));
    }
    else{
        poke16(0x6a, (peek16(0x6a) & ~(1 << pinOffSet)));
    }

    // Make the specified pin into an output in direction register
    poke16(0x6c, peek16(0x6c) | (1 << pinOffSet)); ///
}

/* Function to read what's in DIO 26 (DIO MULTIPLEXER)
 *
 *  Only used for debug purposes.
 *
 * */
unsigned short read_dio26(void){
    int pinOffSet = 5;
    int value_read = 0;

    // Make the specified pin into an input direction register
    poke16(0x6c, peek16(0x6c) & ~(1 << pinOffSet)); ///

    value_read = peek16(0x6a) & (1 << pinOffSet);

    // Make the specified pin into an output in direction register
    poke16(0x6c, peek16(0x6c) | (1 << pinOffSet)); ///

  return value_read;
}


void write_watchdog(void){
    return;
    poke16(WATCHDOG_FPGA_ADDRESS, WATCHDOG_TIME_10SEG);
}
