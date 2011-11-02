# LDFLAGS=-fprofile-arcs
# CFLAGS=-march=armv4 -Os -Wall #-fprofile-arcs -ftest-coverage
# CFLAGS=-Wl,--rpath,/slib -Wl,-dynamic-linker,/slib/ld-uClibc.so.0
CFLAGS=-Wl,--rpath,/slib -Wl,-dynamic-linker,/home/workspace/mia_vita/ts7500_kernel/arm-uclibc-3.4.6/lib/ld-uClibc.so.0 -s
CC=/home/workspace/mia_vita/ts7500_kernel/arm-uclibc-3.4.6/bin/arm-linux-gcc

#  caviumprefix=/home/michael/ts/tools/arm-uclibc-3.4.6/bin/arm-linux-
caviumprefix=/home/workspace/mia_vita/ts7500_kernel/arm-uclibc-3.4.6/bin/arm-linux-
AR_cavium=$(caviumprefix)ar
CC=$(caviumprefix)gcc
LDFLAGS_cavium=-Wl,--rpath,/slib -Wl,-dynamic-linker,/slib/ld-uClibc.so.0
CFLAGS_cavium=-march=armv4 -Os -g

all: sdctl

sdctl: sdctl.c sdcore2.c
	${CC} ${CFLAGS} ${LDFLAGS} sdctl.c -o sdctl

rebuild: clean sdctl

clean:
	rm -rf *.o sdctl 
