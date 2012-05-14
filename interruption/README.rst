Files Structure
===============
This is the file structure::

   .
   ├── README
   ├── fpga.c                 - Communicates with the FPGA
   ├── int.c                  - Main file. Registers the interruption and has most of the logic
   ├── mem_addr.h             - Provides the MEM addresses
   ├── proc_entry.c           - Provides a proc file for retrieving the values
   ├── proc_entry.h           - Just has the function headers for proc_entry
   ├── userland_reader.c      - Userland app to read the data from the created proc entry
   ├── Makefile
   └── modified_binaries
       ├─ sdctl                  - Changed binary to make sdctl not ruin the SPI bus for the interruption
       ├── Makefile
       ├── sdcore2.c
       ├── sdctl.c
       └── README
       ├─ xuartctl               - Changed binary to make xuartctl not ruin the SPI bus for the interruption
       ├── Makefile
       ├── xuartcore.c
       └── xuartctl.c

Userland Reader
===============
Compile with:
   make userland_reader

Userland reader listens to /proc/geophone and converts output with two's complement which is then print to stdout

Instructions to upload and use the module
=========================================

We need to kill this processes in order to limit the access to the spi. In the case of sdctl we do not kill it but use the modified binary in the sdctl directory. (If xuartctl is needed do the same)

Upload the module:

   kill $(pgrep xuartctl); kill $(pgrep daqctl);  kill $(pgrep dioctl); kill -9 $(pgrep logsave); kill $(pgrep ts7500ctl); sleep 2; ts7500ctl --autofeed 3; sleep 5; kill $(pgrep ts7500ctl);

   rmmod int_mod.ko; scp jtrindade@172.20.41.204:/home/workspace/mia_vita/interruption/int_mod.ko .; insmod int_mod.ko;

Using GPS
=========

To compile the with the interruption getting values of the GPS time --> get's the variables __miavita_elapsed_secs and __miavita_elapsed_usecs which are present on the altered kernel (file: ipc/miavita_syscall.c) use the following flag:

EXTRA_CFLAGS+=-D__GPS__


Notes on ADC
============

Data should be **read** on the SPI **rising edge** and changes occur on the SPI failing edge
