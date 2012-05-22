Binaries to replace in the fastboot card
========================================

Checking version
================

If you run xuartctl with the --help parameter you should see the last line:

  MIA-VITA Version 1.0

Compiling XUARTCTL
------------------

You have to use the crosscompiler in your computer. Very important:

1.The flag for the processor in gcc should be -march=armv4 (not armv9) as in the doc

2. You should use the ../../../crosstool/arm-uclibc-3.4.6/ to compile for uClibc and not the normal libc. If you use the normal crosscompiler you will compile for the normal stdlibc.
