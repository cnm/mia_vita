/*
 * =====================================================================================
 *
 *       Filename:  int.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/11/2011 05:47:11 PM
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
#include <linux/delay.h>        /* udelay */
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <asm/io.h>             /* ioremap */

MODULE_AUTHOR ("Joao Trindade");
MODULE_LICENSE("GPL");

#define IRQ_NUMBER 13
#define MEMORY_REGION 0x7C000000
#define MEMORY_SIZE 4

#define MISC_REGISTER 0x76000000
#define GPIOA_EN_ADDRESS    ((MISC_REGISTER) + 0x20)  /* GPIO_A_PIN_ENABLE_REGISTER See page 187 */
#define TEST_ADDR           ((MISC_REGISTER) + 0x18)

#define SCL_BIT_NUMBER 13
#define SDA_BIT_NUMBER 14
#define SCL_MASK (1<<SCL_BIT_NUMBER)
#define SDA_MASK (1<<SCL_BIT_NUMBER)

#define GPIOA_EN_MASK (SCL_MASK | SDA_MASK)

/* For testing purposes  */
/*#undef GPIOA_EN_ADDRESS*/
/*#define GPIOA_EN_ADDRESS TEST_ADDR */

/* Function Headers*/
void release_mem(volatile unsigned int mem_addr, unsigned int memory_size);
void request_memory_regions(void);
irqreturn_t interrupt(int irq, void *dev_id);
void register_handle_interruption(void);
void unregister_handle_interruption(void);
void unregister_memory_region(void);
int request_mem(volatile unsigned int mem_addr, unsigned int size);
int request_port(unsigned int port_addr, unsigned int size);
void enable_pin_interruptions(void);

unsigned int gpioa_en_new_address = 0;

/*
 * To handle the interruption
 */
irqreturn_t interrupt(int irq, void *dev_id)
{
  printk(KERN_INFO "Inside the interruption\n");
  printk(KERN_EMERG "Inside the interruption\n");
  return IRQ_HANDLED;
}

/* Requests all memory necessary for the module */
void request_memory_regions(void){
    unsigned int i;

    gpioa_en_new_address = request_mem(GPIOA_EN_ADDRESS, MEMORY_SIZE);

    i = *(unsigned int *)(gpioa_en_new_address);
    printk(KERN_INFO "Testing with address: %p --------> %x\n", (void *) gpioa_en_new_address, i);
    return;
}

/* Unregister memory regions */
void unregister_memory_region()
{
  release_mem(GPIOA_EN_ADDRESS, MEMORY_SIZE);
}

/*  Set's all pins needed for interruptions */
void enable_pin_interruptions(void)
{


  /* Puts GPIOA_EN bits 13 and 14 to 0 */
  volatile unsigned int *p;
  p = (unsigned int *) gpioa_en_new_address;

  printk(KERN_INFO "\t\tBEFORE: %x \n", *p);
  *p |= GPIOA_EN_MASK;
  printk(KERN_INFO "\t\tAfter: %x \n", *p);

}


/* Runs when the module is initiated */
int init(void)
{
  printk(KERN_INFO "starting interruption module.\n");

  request_memory_regions();
  enable_pin_interruptions();
  register_handle_interruption();



  return 0;
}

/*
 * Runs when the module is unloaded
 * */
void cleanup(void)
{
  unregister_handle_interruption();
  /* unregister_device(); ???? */
  unregister_memory_region();

  printk(KERN_INFO "Unregister module interruption.\n");
}


/*
 * Register the interruption handler (and the IRQ number)
 * */
void register_handle_interruption(){
    int result;

    printk(KERN_INFO "Registering to handle IRQ number %i\n", IRQ_NUMBER);
    result = request_irq(IRQ_NUMBER, interrupt, IRQF_DISABLED, "MV_INT", NULL);
    if (result) {
        printk(KERN_INFO "can't get assigned irq %i\n", IRQ_NUMBER);
        printk(KERN_INFO "Error number result %i\n", result);
    }
}


/**************************** Auxiliary Functions *****************************/
int request_mem(volatile unsigned int mem_addr, unsigned int size){
    unsigned int new_mem;
    if (! request_mem_region(mem_addr, size, "MV_INT")) {
        printk(KERN_INFO "can't get I/O mem address 0x%x\n", mem_addr);
        return -ENODEV;
    }
    new_mem = (unsigned int) ioremap(mem_addr, size);
    printk(KERN_INFO "New memory pointer: %x\n", new_mem);

    return new_mem;
}


int request_port(unsigned int port_addr, unsigned int size){
    unsigned int new_mem = 0;

    if (!request_region(port_addr, size, "MV_INT")) {
        printk(KERN_INFO "can't get I/O port address 0x%x\n", port_addr);
        return -ENODEV;
    }

    return new_mem;
}

void unregister_handle_interruption()
{
  free_irq(IRQ_NUMBER, NULL);
}

void release_port(unsigned int port_addr, unsigned int memory_size)
{
  release_region(port_addr, memory_size);

}

void release_mem(volatile unsigned int mem_addr, unsigned int memory_size)
{
  iounmap((void __iomem *)mem_addr);
  release_mem_region(mem_addr, memory_size);
}

/******************************** End of auxiliary functions **************************/


module_init(init);
module_exit(cleanup);
