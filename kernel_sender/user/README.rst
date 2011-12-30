Important
=========

This must be compiled with a recent toolchain, because functions in endian.h which reverse the byte order are only implemented in recent versions. This one works for me::

    arm-unknown-linux-gnu-gcc --version

    arm-unknown-linux-gnu-gcc (crosstool-NG 1.12.1) 4.5.2
    Copyright (C) 2010 Free Software Foundation, Inc.
    This is free software; see the source for copying conditions.  There is NO
    warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

What is this for?
=================

This program is the server, which receives samples and writes them to a binary and json file.

Initialize the GPS counters in kernel
=====================================

This code also contains a program (init_counter) which will initialize the GPS variable in the modified kernel. (make init_counter).

File Structure
==============
Note: There are two mains here: One to set the time in the kernel through the GPS and one to read packets

.
├── gps_time.c          - Contacts the GPS and ask him the time
├── gps_time.h          - "" 
├── gps_uartctl.c       - UART init, read and write primitives for the GPS
├── gps_uartctl.h       - ""
├── init_counter.c      - Main for reading the GPS and then setting the seconds
├── list.c              - List operation and functions to write to json and raw the packets
├── list.h              - ""
├── macros.h            - Macros for printing debug info
├── main.c              - Opens a socket, reads packets and stores them in a list
├── miavita_packet.h -> ../../kernel_sender/miavita_packet.h
├── syscall_wrapper.c   - Syscalls to set the two time variables in the kernel
└── syscall_wrapper.h   - ""
