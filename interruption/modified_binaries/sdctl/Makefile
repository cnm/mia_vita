# LDFLAGS=-fprofile-arcs
# CFLAGS=-march=armv4 -Os -Wall #-fprofile-arcs -ftest-coverage
# CFLAGS=-Wl,--rpath,/slib -Wl,-dynamic-linker,/slib/ld-uClibc.so.0
# CFLAGS=-Wl,--rpath,/slib -Wl,-dynamic-linker,/home/workspace/mia_vita/ts7500_kernel/arm-uclibc-3.4.6/lib/ld-uClibc.so.0 -s
# CC=/home/workspace/mia_vita/ts7500_kernel/arm-uclibc-3.4.6/bin/arm-linux-gcc

caviumprefix=/home/workspace/mia_vita/crosstool/arm-unknown-linux-gnu/bin/arm-unknown-linux-gnu-
AR_cavium=$(caviumprefix)ar
CC=$(caviumprefix)gcc
STRIP=$(caviumprefix)strip
LDFLAGS=-Wl,--rpath,/slib -Wl,-dynamic-linker,/slib/ld-uClibc.so.0
CFLAGS=-march=armv4 -Os -g

all: sdctl

sdctl: sdctl.c sdcore2.c
	${CC} ${CFLAGS} ${LDFLAGS} sdctl.c -o sdctl
	${STRIP} sdctl

rebuild: clean sdctl

clean:
	rm -rf *.o sdctl 
