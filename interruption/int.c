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

#define IRQ_NUMBER                      4
#define WORD_SIZE                       4

#define MISC_REGISTER                   0x76000000
#define GPIOA_EN_ADDRESS                ((MISC_REGISTER) + 0x20)    /* See page 187 */
#define TEST_ADDR                       ((MISC_REGISTER) + 0x18)

#define SCL_BIT_NUMBER                  13
#define SDA_BIT_NUMBER                  14
#define SCL_MASK                        (1<<SCL_BIT_NUMBER)
#define SDA_MASK                        (1<<SDA_BIT_NUMBER)
#define GPIOA_EN_MASK                   (SCL_MASK | SDA_MASK)

#define GPIOA_REGISTER                  0x7C000000
#define GPIO_DATA_INPUT                 ((GPIOA_REGISTER) + 0x04)   /* See page 223 */
#define PIN_DIR_ADDRESS                 ((GPIOA_REGISTER) + 0x08)   /* See page 223 */
#define INTRENABLE_ADDRESS              ((GPIOA_REGISTER) + 0x20)   /* See page 224 */
#define INTRMASK_ADDRESS                ((GPIOA_REGISTER) + 0x2C)   /* See page 224 */

#define VIC                             0xFFFFF000
#define INT_STATUS_ADDRESS              ((VIC) + 0x00)          /* See page 291 */
#define INT_MASK_ADDRESS                ((VIC) + 0x08)          /* See page 292 */
#define INT_MASK_CLEAR_ADDRESS          ((VIC) + 0x0C)          /* See page 293 */
#define INT_TRIGGER_MODE                ((VIC) + 0x10)          /* See page 293 */
#define INT_TRIGGER_LEVEL               ((VIC) + 0x14)          /* See page 293 */
#define FIQ_SELECT_ADDRESS              ((VIC) + 0x18)          /* See page 293 */
#define IRQ_STATUS                      ((VIC) + 0x1C)          /* See page 294 */
#define IRQ_PRIOTITY                    ((VIC) + 0xC0)          /* See page 296 */
#define VIC_CONTROL                     ((VIC) + 0x144)         /* See page 297 */

#define IRQ_GPIO                        4                       /* See page 291 */
#define IRQ_GPIO_MASK                   (1<<IRQ_GPIO)

#define SOFT_INT_REGISTER               ((VIC) + 0x24)          /* See page 294  */


/* For testing purposes  */
/*#undef GPIOA_EN_ADDRESS*/
/*#define GPIOA_EN_ADDRESS TEST_ADDR */

/* Function Headers*/
void print_priorities(void);
void release_mem(volatile unsigned int mem_addr, unsigned int byte_size);
void request_memory_regions(void);
irqreturn_t interrupt(int irq, void *dev_id);
void register_handle_interruption(void);
void unregister_handle_interruption(void);
void unregister_memory_region(void);
int request_mem(volatile unsigned int mem_addr, unsigned int size);
int request_port(unsigned int port_addr, unsigned int size);
void enable_gpio_interruptions(void);
void enable_irq_interruptions(void);
void test_interrupts(void);
void cleanup(void);

/* Io remap addresses  */
unsigned int gpioa_en_new_address = 0;
unsigned int intr_en_new_address = 0;
unsigned int pin_dir_new_address = 0;
unsigned int intrmask_new_address = 0;
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

/*
 * Functions to handle the interruption
 */
irqreturn_t interrupt(int irq, void *dev_id)
{
  volatile unsigned int *p; // The volatile is extremely important here
  printk(KERN_INFO "Inside the interruption %d\n", irq);
  printk(KERN_EMERG "Inside the interruption %d\n", irq);

  p = (unsigned int *) intrmask_new_address;
  printk(KERN_INFO "\t IntrMask BEFORE: \t\t\t%08x \n", *p);
  *p |= GPIOA_EN_MASK;
  printk(KERN_INFO "\t IntrMask AFTER:  \t\t\t%08x \n", *p);

  return IRQ_HANDLED;
}

/* Requests all memory necessary for the module */
void request_memory_regions(void){

    gpioa_en_new_address = request_mem(GPIOA_EN_ADDRESS, WORD_SIZE);
    pin_dir_new_address = request_mem(PIN_DIR_ADDRESS, WORD_SIZE);
    intr_en_new_address = request_mem(INTRENABLE_ADDRESS, WORD_SIZE);
    intrmask_new_address = request_mem(INTRMASK_ADDRESS, WORD_SIZE);
    int_status_new_address = request_mem(INT_STATUS_ADDRESS, WORD_SIZE);
    int_mask_new_address = request_mem(INT_MASK_ADDRESS, WORD_SIZE);
    int_mask_clear_new_address  = request_mem(INT_MASK_CLEAR_ADDRESS, WORD_SIZE);
    irq_status_new_address = request_mem(IRQ_STATUS, WORD_SIZE);
    fiq_select_new_address = request_mem(FIQ_SELECT_ADDRESS, WORD_SIZE);
    int_trigger_mode_new_address = request_mem(INT_TRIGGER_MODE, WORD_SIZE);
    int_trigger_level_new_address = request_mem(INT_TRIGGER_LEVEL, WORD_SIZE);
    irq_priorities_new_address = request_mem(IRQ_PRIOTITY, WORD_SIZE*32);
    vic_control_new_address = request_mem(VIC_CONTROL, WORD_SIZE);
    sotware_int_new_address = request_mem(SOFT_INT_REGISTER, WORD_SIZE);
    gpio_data_input_new_address = request_mem(GPIO_DATA_INPUT, WORD_SIZE);

    /* Just for tests */
    /*    unsigned int i;*/
    /*    i = *(unsigned int *)(gpioa_en_new_address);*/
    /*    printk(KERN_INFO "Testing with address: %p --------> %x\n", (void *) gpioa_en_new_address, i);*/
    return;
}

/* Unregister memory regions */
void unregister_memory_region()
{
  release_mem(GPIOA_EN_ADDRESS, WORD_SIZE);
  release_mem(PIN_DIR_ADDRESS, WORD_SIZE);
  release_mem(INTRENABLE_ADDRESS, WORD_SIZE);
  release_mem(INTRMASK_ADDRESS, WORD_SIZE);
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
}

/*  Set's all pins needed for interruptions */
void enable_gpio_interruptions(void)
{
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

  /* Puts INTR_EN bits 13 and 14 to 1 - 3.15.16*/
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

void enable_irq_interruptions(void){
  volatile unsigned int *p; // The volatile is extremely important here

  /* Interruption mask read  */
  p = (unsigned int *) int_mask_new_address;
  printk(KERN_INFO "\t IRQ Mask BEFORE:\t\t\t%08x 3.23.3\n", *p);
  /* Enable interruptions for GPIO - Set bit 4 of Interrupt Mas Clear register to 1 */
  p = (unsigned int *) int_mask_clear_new_address;
  *p |= IRQ_GPIO_MASK;
  /* Interruption mask read  */
  p = (unsigned int *) int_mask_new_address;
  printk(KERN_INFO "\t IRQ Mask AFTER:\t\t\t%08x Is losing value each time\n", *p);

  /* Set to level trigger mode - Set bit 4 of OF Interrupt Trigger mode to 0 */
  p = (unsigned int *) int_trigger_mode_new_address;
  printk(KERN_INFO "\t Trigger Mode BEFORE:\t\t\t%08x 3.23.5\n", *p);
  *p |= IRQ_GPIO_MASK; //Not working
  printk(KERN_INFO "\t Trigger Mode after:\t\t\t%08x NOT WORKING 2letter should be odd \n", *p);

  /* IRQ Status read */
  p = (unsigned int *) irq_status_new_address;
  printk(KERN_INFO "\t IRQ status BEFORE:\t\t\t%08x 3.23.8\n", *p);

  /* Set GPIO interruption to be treated by IRQ - Set bit 4 of FIQ select register to 0 */
  p = (unsigned int *) int_mask_clear_new_address;
  *p &= ~IRQ_GPIO_MASK;

  /* IRQ Status read */
  p = (unsigned int *) irq_status_new_address;
  printk(KERN_INFO "\t IRQ status AFTER:\t\t\t%08x Bit 4 should be 0\n", *p);


  /* TEST read */
  p = (unsigned int *) int_trigger_level_new_address;
  printk(KERN_INFO "\t TEST trigger level Before:\t\t%08x 3.23.6 \n", *p);
  *p |= IRQ_GPIO_MASK;
  printk(KERN_INFO "\t TEST trigger level Before:\t\t%08x Error should change from previous\n", *p);

  /*  Test VIC */
  p = (unsigned int *) vic_control_new_address;
  printk(KERN_INFO "\t TEST VIC CONTROL Before:\t\t%08x \n", *p);


  /* Test Software interrupt  */
  p = (unsigned int *) sotware_int_new_address;
  printk(KERN_INFO "\t TEST Read soft int :\t\t%08x \n", *p);
/*  *p |= IRQ_GPIO_MASK;*/
/*  printk(KERN_INFO "\t TEST Read soft int :\t\t%08x \n", *p);*/



}

void print_priorities()
{
  volatile unsigned int *p; // The volatile is extremely important here
  unsigned int i;

  for(i=0;i<32;i++)
    {
      p = (unsigned int *) (irq_priorities_new_address + i*4);
/*      printk(KERN_EMERG "\t\t\tReading address %x %p",(irq_priorities_new_address + i), p);*/

      printk(KERN_INFO "\t\t\tReading priority %p:\t\t%08x \n", p, *p);
    }
}

/*
 * Function which runs when the module is initiated
 * */
int init(void)
{
  printk(KERN_INFO "starting interruption module.\n");

  request_memory_regions();
  register_handle_interruption();
  enable_gpio_interruptions();
/*  print_priorities();*/
  
  test_interrupts();

  return 0;
}

void test_interrupts(void){
  unsigned int * p;
  p = (unsigned int *) gpio_data_input_new_address;
  printk(KERN_INFO "\t GPIO Input register:\t\t\t%08x Is losing value each time\n", *p);

}

/*
 * Runs when the module is unloaded
 * */
void cleanup(void)
{
  unsigned int * p;
  p = (unsigned int *) int_mask_new_address;
  printk(KERN_INFO "\t IRQ Mask AFTER:\t\t\t%08x Is losing value each time\n", *p);
  unregister_handle_interruption();
  printk(KERN_INFO "\t IRQ Mask AFTER:\t\t\t%08x Is losing value each time\n", *p);

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
        printk(KERN_INFO "can't get I/O mem address 0x%08x\n", mem_addr);
        return -ENODEV;
    }
    new_mem = (unsigned int) ioremap(mem_addr, size);
    printk(KERN_INFO "New memory pointer: %08x\n", new_mem);

    return new_mem;
}


int request_port(unsigned int port_addr, unsigned int size){
    unsigned int new_mem = 0;

    if (!request_region(port_addr, size, "MV_INT")) {
        printk(KERN_INFO "can't get I/O port address 0x%08x\n", port_addr);
        return -ENODEV;
    }

    return new_mem;
}

void unregister_handle_interruption()
{
  free_irq(IRQ_NUMBER, NULL);
}

void release_port(unsigned int port_addr, unsigned int byte_size)
{
  release_region(port_addr, byte_size);

}

void release_mem(volatile unsigned int mem_addr, unsigned int byte_size)
{
  iounmap((void __iomem *)mem_addr);
  release_mem_region(mem_addr, byte_size);
}

/******************************** End of auxiliary functions **************************/


module_init(init);
module_exit(cleanup);
