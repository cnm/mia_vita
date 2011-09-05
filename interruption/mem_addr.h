/*
 * =====================================================================================
 *
 *       Filename:  mem_addr.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/18/2011 04:08:17 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  
 *
 * =====================================================================================
 */

/* For spi communication */
#define SPI_REGISTER                    0x71000000
#define SPI_CFG                         0x40   /* See page 144 */
#define SPI_TX_CTRL                     0x4C   /* See page 146 */
#define SPI_RX_DATA                     0x58   /* See page 148 */
#define SPI_FIFO_TX_CTRL                0x60   /* See page 149 */
#define SPI_FIFO_RX_CFG                 0x64   /* See page 149 */
#define SPI_INTR_ENA                    0x6C   /* See page 151 */

#define LUN_CS_ERASE_MASK               (0x700)     /* The bit 8 and 9 is 1 the rest is 0   */
#define LUN_CS_BIT_MASK                 (1<<8)      /* The bit 9 is 1 the rest is 0         */
#define SPEED_MASK                      (7<<10)     /* Mask to reset the speed              */
#define EDGE_MASK                       (1<<14)     /* The edge is set in the 14 bit        */
#define SPEED_SHIFT                     10          /* 10 bits to 13 set the speed          */



/* For interruption */
#define IRQ_NUMBER                      4
#define WORD_SIZE                       4

#define MISC_REGISTER                   0x76000000
#define GPIOA_EN_ADDRESS                ((MISC_REGISTER) + 0x20)    /* See page 187 */
#define TEST_ADDR                       ((MISC_REGISTER) + 0x18)

#define SCL_BIT_NUMBER                  14
#define SDA_BIT_NUMBER                  13
#define SCL_MASK                        (1<<SCL_BIT_NUMBER)
#define SDA_MASK                        (1<<SDA_BIT_NUMBER)
#define GPIOA_EN_MASK                   (SCL_MASK | SDA_MASK)

#define GPIOA_REGISTER                  0x7C000000
#define GPIO_DATA_INPUT                 ((GPIOA_REGISTER) + 0x04)   /* See page 223 */
#define PIN_DIR_ADDRESS                 ((GPIOA_REGISTER) + 0x08)   /* See page 223 */
#define INTRENABLE_ADDRESS              ((GPIOA_REGISTER) + 0x20)   /* See page 224 */
#define GPIO_INT_STATUS                 ((GPIOA_REGISTER) + 0x28)   /* See page 225 */
#define INTRMASK_ADDRESS                ((GPIOA_REGISTER) + 0x2C)   /* See page 225 */
#define GPIO_INT_CLEAR                  ((GPIOA_REGISTER) + 0x30)   /* See page 225 */

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
