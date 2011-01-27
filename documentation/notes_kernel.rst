Communication SPI (Quick and easy)
===========================
The following command would set SPI to use the positive 
clock edge, with a clock frequency of 2Mhz, using LUN #1, and would 
write 0x04:0x00 and then would write 0x01:0x02 while reading and 
outputting to stdout in hex format:

spictl -e 1 -c 2000000 -l 1 -w 04:00 -d 01:02 | hexdump -C

Seen here: http://groups.yahoo.com/group/ts-7000/message/17368;_ylc=X3oDMTM3MGo0dDduBF9TAzk3MzU5NzE0BGdycElkAzEzNjQwMzQ5BGdycHNwSWQDMTcwNTAwNTM3OARtc2dJZAMxOTgxNgRzZWMDZnRyBHNsawN2dHBjBHN0aW1lAzEyOTQ4NTE1NTgEdHBjSWQDMTczNjg-

HZ Freq
=======

If the HZ value for the kernel is 100 then the timing resolution
(not accuracy by the way :-) is 10ms.

One option would be to recompile the kernel with a higher HZ value.
The kernel arranges for a regular clock tick at this freq. and uses the
tick for updating system timers, scheduling etc. So beware, upping the HZ
value will up the system load. A value of 500 may be ok for your purposes.

from: http://tech.groups.yahoo.com/group/ts-7000/message/8367

Get clock resolution (HZ_VALUE)
===============================

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
int main ()
{
    struct timespec resolution, now;
    printf ("Size of __time_t: %d\n", sizeof (__time_t));
    printf ("Size of int: %d\n", sizeof (int));
    clock_getres (CLOCK_REALTIME, &resolution);
    printf ("Resolution %ld sec: %ld nsec\t(%f sec)\n" , (long)resolution.tv_sec, (long)resolution.tv_nsec, (double)resolution.tv_sec + 1.0e-9*resolution.tv_nsec);
    clock_gettime (CLOCK_REALTIME, &now);
    printf ("Time now: %ld:%ld (seconds:nanoseconds)\n", (long)now.tv_sec, (long)now.tv_nsec);
    return 0;
}

Alternative to get lower clock (Usefull for ADC??)
==================================================
There is an alternative that may be suitable. The nanosleep libary call will do busy-loop
wait when invoked with a duration less than the tick interval. This allows you to go down
to microsecond resolution, but your CPU usage will be be a nice level 100%, and other
programs may or may not get a look in. PS -you also have to set your processes
scheduling prority to one of the realtime classes - SCHED_RR or SCHED_FIFO.

Note: Maybe only usefull for 2.4.x kernelso

Interesting discussion regarding timers
=======================================

http://marc.info/?t=118711874700004&r=1&w=2

Particularly this message: http://marc.info/?l=linux-arm-kernel&m=118765994111653&w=2
And this: http://marc.info/?l=linux-arm-kernel&m=118779024232287&w=2

This seems acceptable for my application -- clock_nanosleep() is
obviously the way to do periodic timers.  However, it still seems that
the jitter is still much higher than 2.6.20.

Urls of High resolution timers
==============================

https://export.writer.zoho.com/public/rreginelli/Chapter-5---High-Resolution-Timers-Final1/fullpage

http://elinux.org/High_Resolution_Timers#How_to_detect_if_your_timer_system_supports_high_resolution

Dynamic Tick kernel Explanation
===============================

http://bvargo.net/blog/2010/sep/13/dynamic-ticks-operating-systems/?c=8

http://lwn.net/Articles/223185/

http://elinux.org/Kernel_Timer_Systems

Advice on how to get samples at a frequency rate and DIO comm
=============================================================

http://tech.dir.groups.yahoo.com/group/ts-7000/message/18279

SPI Communication
=================

http://tech.dir.groups.yahoo.com/group/ts-7000/message/19178

Use hardware interrupt to enable precision
==========================================

This maybe very usefull for constant sample rates

http://tech.groups.yahoo.com/group/ts-7000/message/18950

http://tech.groups.yahoo.com/group/ts-7000/message/19261
