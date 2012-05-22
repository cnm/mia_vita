# LDFLAGS=-fprofile-arcs
# CFLAGS=-march=armv4 -Os -Wall #-fprofile-arcs -ftest-coverage
# CFLAGS=-Wl,--rpath,/slib -Wl,-dynamic-linker,/slib/ld-uClibc.so.0
# CFLAGS=-Wl,--rpath,/slib -Wl,-dynamic-linker,/home/workspace/mia_vita/ts7500_kernel/arm-uclibc-3.4.6/lib/ld-uClibc.so.0 -s
# CC=/home/workspace/mia_vita/ts7500_kernel/arm-uclibc-3.4.6/bin/arm-linux-gcc

<<<<<<< HEAD
# caviumprefix=/home/workspace/mia_vita/crosstool/arm-unknown-linux-gnu/bin/arm-unknown-linux-gnu-
caviumprefix=../../../crosstool/arm-uclibc-3.4.6/bin/arm-linux-
=======
caviumprefix=../../../crosstool/arm-unknown-linux-gnu/bin/arm-unknown-linux-gnu-
>>>>>>> bf3798cc5bdea4e7a91feea0919ee1b10069f577
AR_cavium=$(caviumprefix)ar
CC=$(caviumprefix)gcc
STRIP=$(caviumprefix)strip
#THE LAST PART OF THE NEXT LINE WAS REMOVED AND NOW YOU CAN COMPILE WITH THE CROSS COMPILE AND RUN IN THE ARMS
LDFLAGS=-Wl,-rpath,/slib -Wl,--dynamic-linker,/slib/ld-uClibc.so.0
CFLAGS=-Wall -Os -march=armv4


all: sdctl

sdctl: sdctl.c sdcore2.c
	${CC} ${CFLAGS} ${LDFLAGS} sdctl.c -o sdctl
	${STRIP} sdctl

simple: sdctl.c sdcore2.c
	gcc sdctl.c -o sdctl
	echo "Do not forget to strip"

rebuild: clean sdctl

clean:
	rm -rf *.o sdctl 
