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

Install Modules
---------------
1. Go get the code for MIA-VITA at:

    https://github.com/cnm/mia_vita

2. Compile the kernel. Follow instructions here: https://github.com/cnm/ts7500_kernel

3. In the interruption module:
    make

4. Copy the interruption module to the arm device. Follow instructions "Instructions to upload and use the module" in https://github.com/cnm/mia_vita/tree/master/interruption

Hardware connections
--------------------
TODO

Collect the data
----------------

TODO
