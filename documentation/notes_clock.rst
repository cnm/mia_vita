Clock speeds:
=============

Our clock is 3.6864Mhz.

According to page 26 of the ADC manual if we use:

  * Low speed mode
  * CLKDIV set to 0

  - 3.6864 MHz / 512 = 7200 Hz										
  - We have two counters, one counting 15 and the other 144.
  - We are counting 144 as 7200Hz/144 is 50Hz										 
  - We are counting 15 because after a SYNC signal the ADC will be restarting and will not provide DRDY for up to 129 sample periods (page 27).										
  - Update: we decided to shift all the measures before, to make sure that the 50th was not lost due to the GPS PPS reset. Thus our first sample is a 1, all the others every 144 ADC data ready										
