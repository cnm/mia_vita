Clock speeds:
=============

Our clock is 3.6864Mhz.

According to page 26 of the ADC manual if we use:

  * Low speed mode
  * CLKDIV set to 0

We get a sample at each 512 clocks.

3.6864 MHz / 512 = 7200 Hz

We have two counters, one counting 15 and the other 144.

We are counting 144 as 7200Hz/144 is 50Hz

We are counting 15 because after a SYNC signal the ADC gives up to 129 "fake" DRDY wrong values (page 27).

129+15 = 144
