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

#define IRQ_NUMBER          4
#define WORD_SIZE         4

#define MISC_REGISTER       0x76000000
#define GPIOA_EN_ADDRESS    ((MISC_REGISTER) + 0x20)    /* See page 187 */
#define TEST_ADDR           ((MISC_REGISTER) + 0x18)

#define SCL_BIT_NUMBER      13
#define SDA_BIT_NUMBER      14
#define SCL_MASK            (1<<SCL_BIT_NUMBER)
#define SDA_MASK            (1<<SCL_BIT_NUMBER)
#define GPIOA_EN_MASK (SCL_MASK | SDA_MASK)

#define GPIOA_REGISTER      0x7C000000
#define PIN_DIR_ADDRESS     ((GPIOA_REGISTER) + 0x08)   /* See page 223 */
#define INTRENABLE_ADDRESS  ((GPIOA_REGISTER) + 0x20)   /* See page 224 */
#define INTRMASK_ADDRESS    ((GPIOA_REGISTER) + 0x2C)   /* See page 224 */

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
void enable_pin_interruptions(void);
void cleanup(void);

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

/*
 * To handle the interruption
 */
irqreturn_t interrupt(int irq, void *dev_id)
{
  printk(KERN_INFO "Inside the interruption %d\n", irq);
  printk(KERN_EMERG "Inside the interruption %d\n", irq);
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
}

/*  Set's all pins needed for interruptions */
void enable_pin_interruptions(void)
{
  volatile unsigned int *p; // The volatile is extremely important here

  /* Puts GPIOA_EN bits 13 and 14 to 0 */
  p = (unsigned int *) gpioa_en_new_address;
  printk(KERN_INFO "\t\tGPIOA_EN BEFORE: \t\t%08x \n", *p);
  *p &= ~GPIOA_EN_MASK;
  printk(KERN_INFO "\t\tGPIOA_EN AFTER:  \t\t%08x \n", *p);

  /* Puts PIN DIR bits 13 and 14 to 0 */
  p = (unsigned int *) pin_dir_new_address;
  printk(KERN_INFO "\t\tPIN_DIR BEFORE: \t\t%08x \n", *p);
  *p &= ~GPIOA_EN_MASK;
  printk(KERN_INFO "\t\tPIN_DIR AFTER:  \t\t%08x \n", *p);

  /* Puts INTR_EN bits 13 and 14 to 1 */
  p = (unsigned int *) intr_en_new_address;
  printk(KERN_INFO "\t\tINTR_EN BEFORE: \t\t%08x \n", *p);
  *p |= GPIOA_EN_MASK;
  printk(KERN_INFO "\t\tINTR_EN AFTER:  \t\t%08x \n", *p);

  /* Puts IntrMask bits 13 and 14 to 1 */
  p = (unsigned int *) intrmask_new_address;
  printk(KERN_INFO "\t\tIntrMask BEFORE: \t\t%08x \n", *p);
  *p |= GPIOA_EN_MASK;
  printk(KERN_INFO "\t\tIntrMask AFTER:  \t\t%08x \n", *p);

  /* Interruption mask read  */
  p = (unsigned int *) int_mask_new_address;
  printk(KERN_INFO "\t\tStatus register BEFORE:\t\t%08x \n", *p);
  /* Enable interruptions for GPIO - Set bit 4 of Interrupt Mas Clear register to 1 */
  p = (unsigned int *) int_mask_clear_new_address;
  *p |= IRQ_GPIO_MASK;
  /* Interruption mask read  */
  p = (unsigned int *) int_mask_new_address;
  printk(KERN_INFO "\t\tStatus register AFTER:\t\t%08x \n", *p);

  /* Set to level trigger mode - Set bit 4 of OF Interrupt Trigger mode to 0 */
  p = (unsigned int *) int_trigger_mode_new_address;
  printk(KERN_INFO "\t\tTrigger Mode BEFORE:\t\t%08x \n", *p);
  *p |= IRQ_GPIO_MASK; //Not working
  printk(KERN_INFO "\t\tTrigger Mode after:\t\t%08x NOT WORKING 2letter should be odd \n", *p);


  /* IRQ Status read */
  p = (unsigned int *) irq_status_new_address;
  printk(KERN_INFO "\t\tIRQ status BEFORE:\t\t%08x \n", *p);

  /* Set GPIO interruption to be treated by IRQ - Set bit 4 of FIQ select register to 0 */
  p = (unsigned int *) int_mask_clear_new_address;
  *p &= ~IRQ_GPIO_MASK;

  /* IRQ Status read */
  p = (unsigned int *) irq_status_new_address;
  printk(KERN_INFO "\t\tIRQ status AFTER:\t\t%08x \n", *p);


  /* TEST read */
  p = (unsigned int *) int_trigger_level_new_address;
  printk(KERN_INFO "\t\tTEST status Before:\t\t%08x \n", *p);
  *p |= IRQ_GPIO_MASK;
  printk(KERN_INFO "\t\tTEST status Before:\t\t%08x Error should change from previous\n", *p);

  /*  Test */
  p = (unsigned int *) vic_control_new_address;
  printk(KERN_INFO "\t\tTEST status Before:\t\t%08x \n", *p);
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

/* Runs when the module is initiated */
int init(void)
{
  printk(KERN_INFO "starting interruption module.\n");

  request_memory_regions();
  enable_pin_interruptions();
  register_handle_interruption();
  print_priorities();

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
