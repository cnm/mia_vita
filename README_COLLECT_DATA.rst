Simple Instructions to collect data (from the beginning)
========================================================

Install OS
----------

1. Get the DD file.
   ftp://ftp.embeddedarm.com/ts-arm-sbc/ts-7500-linux/binaries/ts-images/2gbsd-noeclipse-sep062011.dd.bz2

2. Confirm it with the md5
   ftp://ftp.embeddedarm.com/ts-arm-sbc/ts-7500-linux/binaries/ts-images/2gbsd-noeclipse-sep062011.dd.bz2.cd5
   md5sum 2gbsd-noeclipse-sep062011.dd.bz2
   cat 2gbsd-noeclipse-sep062011.dd.bz2.md5

3. UnGzip it and dd to the sd card
    bunzip2 2gbsd-noeclipse-sep062011.dd.bz2
    dd if=2gbsd-noeclipse-sep062011.dd of=/dev/yourUSB

4. (Optional do a md5sum to the sdcard)

5. Put JP1=ON JP2=OFF

6. Copy the kernel to the flash drive
  Follow the "Copy the kernel and initrd to the flash in the arm" in https://github.com/cnm/mia_vita
  If you are using the PCB board, you do not have to install anything in the flash drive (it runs everything from the sd card)

Make the Interruption Module
----------------------------

1. Go get the code for MIA-VITA at:

    https://github.com/cnm/mia_vita

2. Compile the kernel. Follow instructions here: https://github.com/cnm/ts7500_kernel

3. In the interruption module (interruption directory):
    make

4. Copy the interruption module to the arm device. Follow instructions "Instructions to upload and use the module" in https://github.com/cnm/mia_vita/tree/master/interruption


Install modified binaries
-------------------------

1. In directories interruption/modified_binaries compile sdctl and xuartctl make both binaries.

2. Copy both binaries (sdctl and xuartctl) to the third partition of the sdcard (directory sbin). (this replaces the olde binaries).

3. If you are not using the PCB card, you have to save this changes to the flash once again.

Hardware connections
--------------------
1. Put the ARM device.

2. Connect the PICO-PSU

3. Press the button (still not working)

4. Connect the Geophone

Collect the data
----------------

1. Go get the interruption module binary (this is how I do it):

..

    IP=172.20.41.95

    scp jtrindade@$IP:/home/workspace/mia_vita/interruption/int_mod.ko .; 

2. Kill a bunch of processes that we don't need (and that use the sdcard, making us lose more samples). Also suspend the watchdog.

   kill $(pgrep xuartctl); kill $(pgrep daqctl);  kill $(pgrep dioctl); kill -9 $(pgrep logsave); kill $(pgrep ts7500ctl); sleep 2; ts7500ctl --autofeed 3; sleep 5; kill $(pgrep ts7500ctl);

3. Put the interrupt module:
   rmmod int_mod; insmod int_mod.ko

4. If you want check the module init with dmesg

5. Go get the userland_reader:

    scp jtrindade@$IP:/home/workspace/mia_vita/interruption/reader .;

6. Run the userland reader
