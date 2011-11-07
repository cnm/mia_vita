# LDFLAGS=-fprofile-arcs
# CFLAGS=-march=armv4 -Os -Wall #-fprofile-arcs -ftest-coverage
# CFLAGS=-Wl,--rpath,/slib -Wl,-dynamic-linker,/slib/ld-uClibc.so.0
# CFLAGS=-Wl,--rpath,/slib -Wl,-dynamic-linker,/home/workspace/mia_vita/ts7500_kernel/arm-uclibc-3.4.6/lib/ld-uClibc.so.0 -s
# CC=/home/workspace/mia_vita/ts7500_kernel/arm-uclibc-3.4.6/bin/arm-linux-gcc

caviumprefix=/home/workspace/mia_vita/ts7500_kernel/arm-uclibc-3.4.6/bin/arm-linux-
AR_cavium=$(caviumprefix)ar
CC=$(caviumprefix)gcc
STRIP=$(caviumprefix)strip
LDFLAGS=-Wl,--rpath,/slib -Wl,-dynamic-linker,/slib/ld-uClibc.so.0
CFLAGS=-march=armv4 -Os -g -lutil

all: xuartctl

xuartctl: xuartctl.c xuartcore.c
	${CC} ${CFLAGS} ${LDFLAGS} xuartctl.c -o xuartctl
	${STRIP} xuartctl

rebuild: clean xuartctl

clean:
	rm -rf *.o xuartctl
