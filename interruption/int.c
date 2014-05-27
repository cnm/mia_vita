#include <linux/module.h>
#include <linux/kernel.h>       /*  printk() */
#include <linux/delay.h>        /* udelay */
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <asm/io.h>             /* ioremap */
#include <linux/miavita_xtime.h>

#include "mem_addr.h"
#include "proc_entry.h"
#include "fpga.h"

MODULE_AUTHOR("Joao Trindade");
MODULE_LICENSE("GPL");

/* Function Headers*/
void release_mem(volatile unsigned int mem_addr, unsigned int byte_size);
void request_memory_regions(void);
irqreturn_t interrupt(int irq, void *dev_id);
void register_handle_interruption(void);
void unregister_handle_interruption(void);
void unregister_memory_region(void);
int request_mem(volatile unsigned int mem_addr, unsigned int size);
int request_port(unsigned int port_addr, unsigned int size);
void enable_gpio_interruptions(void);
void cleanup(void);
void handle_gps_int(void);

void handle_adc_int(void);

/* Io remap addresses  */
unsigned int gpioa_en_new_address = 0;
unsigned int intr_en_new_address = 0;
unsigned int pin_dir_new_address = 0;
unsigned int intrmask_new_address = 0;
unsigned int intr_trigger_new_address = 0;
unsigned int intr_both_new_address = 0;
unsigned int intr_rise_neg_new_address = 0;
unsigned int int_status_new_address = 0;
unsigned int int_mask_new_address = 0;
unsigned int int_mask_clear_new_address = 0;
unsigned int irq_status_new_address = 0;
unsigned int fiq_select_new_address = 0;
unsigned int int_trigger_mode_new_address = 0;
unsigned int int_trigger_level_new_address = 0;
unsigned int irq_priorities_new_address = 0;
unsigned int vic_control_new_address = 0;
unsigned int sotware_int_new_address = 0;
unsigned int gpio_data_input_new_address = 0;
unsigned int gpio_int_clear_new_address = 0;
unsigned int gpio_int_status_new_address = 0;

unsigned int counter_sda = 0;
unsigned int counter_scl = 0;
unsigned int counter_seconds = 0;

extern void release_mem_spi(void);
extern void read_four_channels(unsigned int * read_buffer, int64_t* timestamp);
extern void prepare_spi(void);
extern void prepare_spi2(void);
extern void write_dio26(bool b);
extern unsigned short read_dio26(void);
extern void write_watchdog(void);

bool is_fpga_used(void);

unsigned int counter;
volatile unsigned short mux_state = 0;
int udelay_in_second;

#define DIVISOR 1

/************************ Configuration Functions *************************/
/* Requests all memory necessary for the module */
void request_memory_regions(void){
    gpioa_en_new_address = request_mem(GPIOA_EN_ADDRESS, WORD_SIZE);
    pin_dir_new_address = request_mem(PIN_DIR_ADDRESS, WORD_SIZE);
    intr_en_new_address = request_mem(INTRENABLE_ADDRESS, WORD_SIZE);
    intrmask_new_address = request_mem(INTRMASK_ADDRESS, WORD_SIZE);
    int_status_new_address = request_mem(INT_STATUS_ADDRESS, WORD_SIZE);
    int_mask_new_address = request_mem(INT_MASK_ADDRESS, WORD_SIZE);
    intr_trigger_new_address = request_mem(INTRTRIGGER_ADDRESS, WORD_SIZE);
    intr_both_new_address = request_mem(INTRBOTH_ADDRESS, WORD_SIZE);
    intr_rise_neg_new_address = request_mem(INTRRISE_ADDRESS, WORD_SIZE);
    int_mask_clear_new_address  = request_mem(INT_MASK_CLEAR_ADDRESS, WORD_SIZE);
    irq_status_new_address = request_mem(IRQ_STATUS, WORD_SIZE);
    fiq_select_new_address = request_mem(FIQ_SELECT_ADDRESS, WORD_SIZE);
    int_trigger_mode_new_address = request_mem(INT_TRIGGER_MODE, WORD_SIZE);
    int_trigger_level_new_address = request_mem(INT_TRIGGER_LEVEL, WORD_SIZE);
    irq_priorities_new_address = request_mem(IRQ_PRIOTITY, WORD_SIZE*32);
    vic_control_new_address = request_mem(VIC_CONTROL, WORD_SIZE);
    sotware_int_new_address = request_mem(SOFT_INT_REGISTER, WORD_SIZE);
    gpio_data_input_new_address = request_mem(GPIO_DATA_INPUT, WORD_SIZE);
    gpio_int_clear_new_address  = request_mem(GPIO_INT_CLEAR, WORD_SIZE);
    gpio_int_status_new_address = request_mem(GPIO_INT_STATUS, WORD_SIZE);

    return;
}

/* Unregister memory regions */
void unregister_memory_region(){
    release_mem(GPIOA_EN_ADDRESS, WORD_SIZE);
    release_mem(PIN_DIR_ADDRESS, WORD_SIZE);
    release_mem(INTRENABLE_ADDRESS, WORD_SIZE);
    release_mem(INTRMASK_ADDRESS, WORD_SIZE);
    release_mem(INTRTRIGGER_ADDRESS, WORD_SIZE);
    release_mem(INTRBOTH_ADDRESS, WORD_SIZE);
    release_mem(INTRRISE_ADDRESS, WORD_SIZE);
    release_mem(INT_STATUS_ADDRESS, WORD_SIZE);
    release_mem(INT_MASK_ADDRESS, WORD_SIZE);
    release_mem(INT_MASK_CLEAR_ADDRESS, WORD_SIZE);
    release_mem(FIQ_SELECT_ADDRESS, WORD_SIZE);
    release_mem(IRQ_STATUS, WORD_SIZE);
    release_mem(INT_TRIGGER_MODE, WORD_SIZE);
    release_mem(INT_TRIGGER_LEVEL, WORD_SIZE);
    release_mem(IRQ_PRIOTITY, WORD_SIZE*32);
    release_mem(VIC_CONTROL, WORD_SIZE);
    release_mem(SOFT_INT_REGISTER, WORD_SIZE);
    release_mem(GPIO_DATA_INPUT, WORD_SIZE);
    release_mem(GPIO_INT_CLEAR, WORD_SIZE);
    release_mem(GPIO_INT_STATUS, WORD_SIZE);
}

/*  Set's all pins needed for GPIO interruptions */
void enable_gpio_interruptions(void){
    volatile unsigned int *p; // The volatile is extremely important here

    /* Puts PIN DIR bits 13 and 14 to 0 - 3.15.3 */
    p = (unsigned int *) pin_dir_new_address;
    printk(KERN_INFO "\t PIN_DIR BEFORE: \t\t\t%08x \n", *p);
    *p &= ~GPIOA_EN_MASK;
    printk(KERN_INFO "\t PIN_DIR AFTER:  \t\t\t%08x \n", *p);

    /* Puts IntrMask bits 13 and 14 to 0 -  3.15.9 */
    p = (unsigned int *) intrmask_new_address;
    printk(KERN_INFO "\t IntrMask BEFORE: \t\t\t%08x \n", *p);
    *p &= ~GPIOA_EN_MASK;
    printk(KERN_INFO "\t IntrMask AFTER:  \t\t\t%08x \n", *p);

    /* Puts Intrtrigger bits 13 and 14 to 0 (edge trigger) -  3.15.11 */
    p = (unsigned int *) intr_trigger_new_address;
    printk(KERN_INFO "\t IntrTrigger BEFORE: \t\t\t%08x \n", *p);
    *p &= ~GPIOA_EN_MASK;
    printk(KERN_INFO "\t IntrTrigger AFTER:  \t\t\t%08x \n", *p);

    /* Puts Intrtrigger bits 13 and 14 to 0 (edge trigger) -  3.15.12 */
    p = (unsigned int *) intr_both_new_address;
    printk(KERN_INFO "\t IntrBoth BEFORE: \t\t\t%08x \n", *p);
    *p &= ~GPIOA_EN_MASK;
    printk(KERN_INFO "\t IntrBoth AFTER:  \t\t\t%08x \n", *p);

    /* Puts Intrtrigger bits 13 and 14 to 0 (rising edge) -  3.15.13 */
    p = (unsigned int *) intr_rise_neg_new_address;
    printk(KERN_INFO "\t IntrRise BEFORE: \t\t\t%08x \n", *p);
    *p &= ~GPIOA_EN_MASK;
    printk(KERN_INFO "\t IntrRise AFTER:  \t\t\t%08x \n", *p);

    /* Puts INTR_EN bits 13 and 14 to 1 - 3.15.6*/
    p = (unsigned int *) intr_en_new_address;
    printk(KERN_INFO "\t INTR_EN BEFORE: \t\t\t%08x \n", *p);
    *p |= GPIOA_EN_MASK;
    printk(KERN_INFO "\t INTR_EN AFTER:  \t\t\t%08x \n", *p);

    /* Puts GPIOA_EN bits 13 and 14 to 0 */
    p = (unsigned int *) gpioa_en_new_address;
    printk(KERN_INFO "\t GPIOA_EN BEFORE: \t\t\t%08x \n", *p);
    *p &= ~GPIOA_EN_MASK;
    printk(KERN_INFO "\t GPIOA_EN AFTER:  \t\t\t%08x \n", *p);

    printk(KERN_INFO "\t\t\t\t ### END GPIO ###\n");
}

/* Function which runs when the module is initiated */
int init(void){
    printk(KERN_INFO "starting interruption module.\n");

    prepare_spi();
    request_memory_regions();
    prepare_spi2();
    register_handle_interruption();
    enable_gpio_interruptions();

    create_proc_file();

    return 0;
}

/* Runs when the module is unloaded */
void cleanup(void){
    unsigned int * p;
    p = (unsigned int *) int_mask_new_address;
    unregister_handle_interruption();

    printk(KERN_INFO "MEMORY BEFORE.\n");
    unregister_memory_region();
    printk(KERN_INFO "MEMORY AFTER.\n");

    release_mem_spi();
    printk(KERN_INFO "Unregistered module interruption.\n");
}

/* Register the interruption handler (and the IRQ number) */
void register_handle_interruption(){
    int result;
    printk(KERN_INFO "Registering to handle IRQ number %i\n", IRQ_NUMBER);
    result = request_irq(IRQ_NUMBER, interrupt, IRQF_DISABLED, "MV_INT", NULL);
    if (result)
      {
        printk(KERN_INFO "can't get assigned irq %i\n", IRQ_NUMBER);
        printk(KERN_INFO "Error number result %i\n", result);
      }
}
/************************ End Configuration Functions *************************/

/**************************** Auxiliary Functions *****************************/
/* Function to be used when requesting physical memory (not virtual).
 *
 * Returns the mem_address (virtual) of the physical memory
 * */
int request_mem(volatile unsigned int mem_addr, unsigned int size){
    unsigned int new_mem;
    if (! request_mem_region(mem_addr, size, "MV_INT")) {
        printk(KERN_INFO "can't get I/O mem address 0x%08x\n", mem_addr);
        return -ENODEV;
    }
    new_mem = (unsigned int) ioremap(mem_addr, size);
    printk(KERN_INFO "New memory pointer: %08x\n", new_mem);

    return new_mem;
}

/* Function to be used when requesting ports.
 *
 * Returns the mem_address (virtual) of the port)
 * */
int request_port(unsigned int port_addr, unsigned int size){
    unsigned int new_mem = 0;

    if (!request_region(port_addr, size, "MV_INT")) {
        printk(KERN_INFO "can't get I/O port address 0x%08x\n", port_addr);
        return -ENODEV;
    }

    return new_mem;
}

void unregister_handle_interruption(){
    free_irq(IRQ_NUMBER, NULL);
}

void release_port(unsigned int port_addr, unsigned int byte_size){
    release_region(port_addr, byte_size);
}

void release_mem(volatile unsigned int mem_addr, unsigned int byte_size){
    iounmap((void __iomem *)mem_addr);
    release_mem_region(mem_addr, byte_size);
}

bool is_fpga_used(void){
    volatile unsigned int *p; // The volatile is extremely important here

    p = (unsigned int *) intr_trigger_new_address;
    return (*p &= 0x1); //Check if the the "MUTEX" bit is set
}
/******************************** End of auxiliary functions **************************/

/******************************** Interruption handlers *******************************/
/* Handle the received interruption*/
irqreturn_t interrupt(int irq, void *dev_id){
    volatile unsigned int *p; // The volatile is extremely important here

    /* Disable interruptions */
    p = (unsigned int *) intr_en_new_address;
    *p &= ~GPIOA_EN_MASK;

    /* Check what the interruption was*/
    p = (unsigned int *) gpio_int_status_new_address;

    /* If scl interruption */
    if(SCL_MASK & *p)
      {
        counter_scl++;

        if((counter_scl % DIVISOR) == 0)
          {
            /* if((counter_scl % 40) == 0){ */
            /*     printk(KERN_INFO "Received adc int ADC\n"); */
            /* } */

            handle_adc_int();
          }
      }


    /* If sda interruption */
    else if(SDA_MASK & *p)
      {
        counter_sda++;

        if((counter_sda % DIVISOR) == 0){
            /* printk(KERN_INFO "Received PPS\n"); */
            handle_gps_int();
        }
      }

    else
      { // should not happen
        printk(KERN_EMERG "------------- ERROR Received unknown interruption ------------------- ??\n");
      }

    /* Clear the GPIO interruption */
    p = (unsigned int *) gpio_int_clear_new_address;
    *p |= GPIOA_EN_MASK;

    /* Enable interruptions */
    p = (unsigned int *) intr_en_new_address;
    *p |= GPIOA_EN_MASK;

    return IRQ_HANDLED;
}

void handle_gps_int(void){

    /* TODO - Change to use a base and then compare the base with the kernel time. Do NOT change the kernel time */
    /* #warning Now I'm changing the kernel own time. Please create a base and then compare the base with the kernel time */
    /* struct timeval t; */
    /* do_gettimeofday(&t); */
    /* t.tv_usec = 0; */

    counter = 0;
    counter_seconds++;
    udelay_in_second = 0;
    __miavita_elapsed_usecs = 0;
    __miavita_elapsed_secs++;

    if(is_fpga_used()){
        return;
    }
    else{
        write_dio26(0);
        mux_state = 0;
        write_watchdog();
    }
    return;
}

#define SAMPLE_RATE_TIME_INTERVAL_U   20000            /* Supposed to be          -> 50Hz -> 20 Miliseconds -> 20 000 Micro+|*/
/* #define SAMPLE_RATE_TIME_INTERVAL_U   13513         /1* Due to error in PCB 74Hz -> 20 Miliseconds -> 20 000 Micro*/
#define DATA_READY_TIME_U                13         /* First sample difference  -> 1 / (3.6??? Mhz / 512) TODO - Calculate this */

void handle_adc_int(){
    unsigned int value_buffer[3];
    bool fpga_busy = is_fpga_used();
    int64_t timestamp;

    /* TODO - Current solution discards next 2 lines but they are extremely usefull if GPS does not find a signal. Maybe pass as a parameter as module is inserted??? */
    struct timeval t;
    __miavita_elapsed_usecs += SAMPLE_RATE_TIME_INTERVAL_U;

    /* TODO - Change to use a base and then compare the base with the kernel time. Do NOT change the kernel time */
    /* #warning Now I'm changing the kernel own time. Please create a base and then compare the base with the kernel time */
    do_gettimeofday(&t);
    /* __miavita_elapsed_usecs = t.tv_usec; */

    counter++;
    if(fpga_busy)
      {
        /* printk(KERN_EMERG "Second %u\tFPGA being used and I'm on the ADC\n", counter_seconds); */
        return;
      }

    if (mux_state == 0)
      {
        write_dio26(1);
        mux_state = 1;

        /* TODO - This lines can be usefull if no GPS is found. But we have to decide if we accept it */
        /* if(counter != 1) udelay_in_second = -15 + (counter -1) * (SAMPLE_RATE_TIME_INTERVAL_U - DATA_READY_TIME_U); */
        /* if(counter != 1) __miavita_elapsed_usecs = -15 + (counter -1) * (SAMPLE_RATE_TIME_INTERVAL_U - DATA_READY_TIME_U); */
      }

    /* Read the adc  */
    read_four_channels(value_buffer, &timestamp);

    /* Save to a buffer the value */
    write_to_buffer(value_buffer, timestamp, counter);
}
/******************************** End of Interruption handlers ************************/

module_init(init);
module_exit(cleanup);
