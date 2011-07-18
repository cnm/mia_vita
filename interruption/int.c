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

MODULE_AUTHOR ("Joao Trindade");
MODULE_LICENSE("GPL");

#define IRQ_NUMBER 7
#define MEMORY_REGION 0x7C000000
#define MEMORY_SIZE 1

void request_memory_regions(void);
irqreturn_t short_interrupt(int irq, void *dev_id);
void register_handle_interruption(void);
void unregister_handle_interruption(void);
void unregister_memory_region(void);

irqreturn_t short_interrupt(int irq, void *dev_id)
{
  printk(KERN_INFO "Inside the interruption\n");
  printk(KERN_EMERG "Inside the interruption\n");
  return IRQ_HANDLED;
}

void request_memory_regions(){
    /* if using ports */
    /*    if (! request_region(MEMORY_REGION, MEMORY_SIZE, "MV_INT")) {
          printk(KERN_INFO "short: can't get I/O port address 0x%lx\n",
          short_base);
          return -ENODEV;
          }*/

    /* If using memory */
    /* if (! request_mem_region(MEMORY_REGION, MEMORY_SIZE, "MV_INT")) {
       printk(KERN_INFO "short: can't get I/O mem address 0x%lx\n",
       short_base);
       return -ENODEV;
       }
       short_base = (unsigned long) ioremap(short_base, SHORT_NR_PORTS);
     */

    return;
}

void register_handle_interruption(){
    int result;
    printk(KERN_INFO "Preparing interruption number 2 %i\n", IRQ_NUMBER);

    result = request_irq(IRQ_NUMBER, short_interrupt, IRQF_DISABLED, "MV_INT", NULL);
    if (result) {
        printk(KERN_INFO "short: can't get assigned irq %i\n",
               IRQ_NUMBER);
    }
}

/* Finally, init and cleanup */
int short_init(void)
{
  printk(KERN_INFO "starting int.\n");

  request_memory_regions();

  /* Here we register our device - should not fail thereafter */
  //register_device()???*/
  /* Request to handle the interruption */
  register_handle_interruption();

  return 0;
}

void unregister_handle_interruption(){
    free_irq(IRQ_NUMBER, NULL);
}

void unregister_memory_region(){
    /*  If I use memory */
    /*
       iounmap((void __iomem *)MEMORY_REGION);
       release_mem_region(MEMORY_REGION, MEMORY_SIZE);
     */

    /*release_region(MEMORY_REGION, MEMORY_SIZE);*/
}

void short_cleanup(void)
{
  unregister_handle_interruption();
  /* unregister_device(); ???? */
  unregister_memory_region();

  printk(KERN_INFO "leaning int.\n");
}

module_init(short_init);
module_exit(short_cleanup);
