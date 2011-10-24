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
#include "mem_addr.h"

extern int request_mem(volatile unsigned int mem_addr, unsigned int size);
extern void release_mem(volatile unsigned int mem_addr, unsigned int byte_size);
void write_dio26(bool b);
void release_mem_spi(void);

extern unsigned int gpioa_en_new_address;

void set_lun_speed_edge(void);
unsigned short getR0(void);
void cavium_disable_cs(void);
int dostuff(void);
void prepare_registers(void);
static volatile unsigned int gpio_a_new_mem;
static volatile unsigned int spi_register;

static volatile unsigned int *cvspiregs;
static void prepare_registers2(void);

void cavium_poke16(unsigned int adr, unsigned short dat) {
    unsigned int dummy = -1;
    unsigned int d = dat;

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

    /*    printk("\tPOKE16 dat=%04X,adr=%04X\n",dat,adr);*/
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
                  : "+r"(ret) 
                  : "r"(adr), "r"(cvspiregs)
                  : "r1", "cc"
    );
    /*    printk(KERN_EMERG "\tPEEK16 dat=%04X,adr=%04X,mem=%p\n",ret,adr,cvspiregs);*/
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

    p = (unsigned int *) (spi_register + SPI_FIFO_TX_CTRL);
    *p = 0x0;        /* disable interrupts */

    p = (unsigned int *) (spi_register + SPI_INTR_ENA);
    *p = 0x4;        /* deassert CS# */

    p = (unsigned int *) (spi_register + SPI_RX_DATA);
    for (i = 0; i < 8; i++) *p;

    p = (unsigned int *) gpio_a_new_mem;
    *p = (2<<15|1<<17|1<<3); /* Enable I2SWS, I2SCLK and I2SDR */

    cavium_disable_cs(); // force CS# deassertion just in case
}

void prepare_registers2() {
    volatile unsigned int *p; // The volatile is extremely important here

    p = (unsigned int *) gpio_a_new_mem;
    *p = (2<<15|1<<17|1<<3); /* Enable I2SWS, I2SCLK and I2SDR */
}

void reserve_memory(void){
    gpio_a_new_mem = request_mem(GPIOA_REGISTER, WORD_SIZE);
    spi_register = request_mem(SPI_REGISTER, 0x6C + WORD_SIZE);
    cvspiregs = (void*) spi_register;
}

void prepare_spi(void){
    printk("Reserving memory\n");
    reserve_memory();

    printk("Preparing registers\n");
    prepare_registers();
    printk("Ended Preparing registers\n");

    printk("Setting parameters\n");
    set_lun_speed_edge();
    printk("End setting parameters\n");
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

unsigned int read_32_bits(void){
    unsigned int ret;
    unsigned short l, h;

    prepare_registers2();
    /*    set_lun_speed_edge();*/ /* VOLTAR A METER  */

    l = cavium_peek16(0x0A);
    h = cavium_peek16(0x0C);

    ret = (h|(l<<16));
    return ret;
}

void release_mem_spi(void){
    release_mem(GPIOA_REGISTER, WORD_SIZE);
    release_mem(SPI_REGISTER, 0x6C + WORD_SIZE);
}


unsigned short sbus_peek16(unsigned int adr) {
    unsigned short ret = 0;
    volatile unsigned int *p; // The volatile is extremely important here
    p = (unsigned int *) gpioa_en_new_address;

    *p = (3<<14); /* Enable I2SWS, I2SCLK and I2SDR */

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
                  : "+r"(ret) : "r"(adr), "r"(cvspiregs) : "r1", "cc"
    );

    *p = (2<<15|1<<17|1<<3); /* Enable I2SWS, I2SCLK and I2SDR */

    return ret;
}

void sbus_poke16(unsigned int adr, unsigned short dat) {
    unsigned int dummy = 0;
    unsigned int d = dat;

    volatile unsigned int *p; // The volatile is extremely important here
    p = (unsigned int *) gpioa_en_new_address;

    *p = (3<<14); /* Enable I2SWS, I2SCLK and I2SDR */

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
                  : "+r"(dummy) : "r"(adr), "r"(d), "r"(cvspiregs) : "r1","cc"
    );

    *p = (2<<15|1<<17|1<<3); /* Enable I2SWS, I2SCLK and I2SDR */
}

void write_dio26(bool b){
    int pinOffSet = 5;

    if(b){
        sbus_poke16(0x6a, (sbus_peek16(0x6a) | (1 << pinOffSet)));
    }
    else{
        sbus_poke16(0x6a, (sbus_peek16(0x6a) & ~(1 << pinOffSet)));
    }

    // Make the specified pin into an output in direction register
    sbus_poke16(0x6c, sbus_peek16(0x6c) | (1 << pinOffSet)); ///
}


