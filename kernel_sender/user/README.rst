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
