/*
 * =====================================================================================
 *
 *       Filename:  bitbang.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  06/09/10 14:15:25
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  
 *
 * =====================================================================================
 */

// transmit byte serially, MSB first
void send_8bit_serial_data(unsigned char data)
{
  unsigned char i;

  // select device
  output_high(SD_CS); //Basically does chip enable

  // send bits 7..0
  for(i = 0; i < 8; i++) //Only for 8bits
    {
      // consider leftmost bit
      // set line high if bit is 1, low if bit is 0
      if (data & 0x80) //It's start in the eighs bith
        output_high(SD_DI);
      else
        output_low(SD_DI);

      // pulse clock to indicate that bit value should be read
      output_low(SD_CLK);
      output_high(SD_CLK);

      // shift byte left so next bit will be leftmost
      data <<= 1;
    }

  // deselect device
  output_low(SD_CS);
}
