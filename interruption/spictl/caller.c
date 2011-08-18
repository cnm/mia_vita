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
void release_mem_spi(void);


void set_lun_speed_edge(void);
unsigned short getR0(void);
void cavium_disable_cs(void);
int dostuff(void);
void prepare_registers(void);
void release_mem_spi(void);

unsigned int gpio_new_mem;
unsigned int spi_cfg_new_mem;
unsigned int spi_fifo_rx_cfd_new_mem;
unsigned int spi_fifo_tx_ctrl_new_mem;
unsigned int spi_intr_ena_new_mem;
unsigned int spi_tx_ctrl_new_mem;
unsigned int spi_rx_data_new_mem;



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
                  : "r"(adr), "r"(d), "r"(spi_new_mem) 
                  : "r1","cc"
    );

    printk("\tPOKE16 dat=%04X,adr=%04X\n",dat,adr);
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
                  : "r"(adr), "r"(spi_new_mem) 
                  : "r1", "cc"
    );
    printk("\tPEEK16 dat=%04X,adr=%04X,\n",ret,adr);
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
        printk("cavium_disable_cs:%04X\n",val);
        setR0(val & ~(1<<7));
        setR0((val & ~(1<<7)) ^ (1 << 14));
    }
    else {
        printk("cavium_disable_cs:%04X (NOP)\n",val);
    }
}

void prepare_registers() {
    int i;
    volatile unsigned int *p; // The volatile is extremely important here

    p = (unsigned int *) spi_fifo_rx_cfd_new_mem;
    *p = 0x0;        /* RX IRQ threshold 0 */

    p = (unsigned int *) spi_cfg_new_mem;
    *p = 0x80000c02; /* 24-bit mode, no byte swap */

    p = (unsigned int *) spi_fifo_tx_ctrl_new_mem;
    *p  = 0x0;        /* 0 clock inter-transfer delay */

    p = (unsigned int *) spi_intr_ena_new_mem;
    *p = 0x0;        /* disable interrupts */

    p = (unsigned int *) spi_tx_ctrl_new_mem;
    *p = 0x4;        /* deassert CS# */

    p = (unsigned int *) spi_rx_data_new_mem;
    for (i = 0; i < 8; i++) *p;

    p = (unsigned int *) gpio_new_mem;
    *p = (2<<15|1<<17|1<<3);

    cavium_disable_cs(); // force CS# deassertion just in case
}


void reserve_memory(void){
    gpio_new_mem = request_mem(GPIOA_REGISTER, WORD_SIZE);

    spi_cfg_new_mem = request_mem(SPI_CFG, WORD_SIZE);
    spi_fifo_rx_cfd_new_mem =  request_mem(SPI_FIFO_RX_CFG, WORD_SIZE);
    spi_fifo_tx_ctrl_new_mem = request_mem(SPI_FIFO_TX_CTRL, WORD_SIZE);
    spi_intr_ena_new_mem = request_mem(SPI_INTR_ENA, WORD_SIZE);
    spi_tx_ctrl_new_mem = request_mem(SPI_TX_CTRL, WORD_SIZE);
    spi_rx_data_new_mem = request_mem(SPI_RX_DATA, WORD_SIZE);
}

void prepare(void){
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
    int edge = 1;

    unsigned int mask = 0;
    unsigned int conf = getR0();

    /* Set the edge */
    if (edge == 1)
      mask |= EDGE_MASK;
    else if (edge == 0)
      mask &= ~EDGE_MASK;
    printk("Writing the edge\n");
    setR0(conf | mask);

    /* Set the lun  */
    mask |= LUN_CS_BIT_MASK;
    conf &= ~LUN_CS_ERASE_MASK;

    /* Set the speed  */
    mask |= clock << SPEED_SHIFT;

    printk("Writing the speed and lun\n");
    setR0(conf | mask);


    printk("MASCARA %X\n", mask);
    printk("CONF: %X\n", conf);
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
    printk("Resultado %u\n", result);

    printk("\n");

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
    printk("Resultado %u\n", result);
    printk("\n");

    return 0;
}

void release_mem_spi(){
    release_mem(GPIOA_REGISTER, WORD_SIZE);

    release_mem(SPI_CFG, WORD_SIZE);
    release_mem(SPI_TX_CTRL, WORD_SIZE);
    release_mem(SPI_RX_DATA, WORD_SIZE);
    release_mem(SPI_FIFO_TX_CTRL, WORD_SIZE);
    release_mem(SPI_FIFO_RX_CFG, WORD_SIZE);
    release_mem(SPI_INTR_ENA, WORD_SIZE);
}
#endif
