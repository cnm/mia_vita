# LDFLAGS=-fprofile-arcs
# CFLAGS=-march=armv4 -Os -Wall #-fprofile-arcs -ftest-coverage
# CFLAGS=-Wl,--rpath,/slib -Wl,-dynamic-linker,/slib/ld-uClibc.so.0
# CFLAGS=-Wl,--rpath,/slib -Wl,-dynamic-linker,/home/workspace/mia_vita/ts7500_kernel/arm-uclibc-3.4.6/lib/ld-uClibc.so.0 -s
# CC=/home/workspace/mia_vita/ts7500_kernel/arm-uclibc-3.4.6/bin/arm-linux-gcc


caviumprefix=/home/workspace/mia_vita/crosstool/arm-unknown-linux-gnu/bin/arm-unknown-linux-gnu-
# caviumprefix=../../../crosstool/arm-uclibc-3.4.6/bin/arm-linux-
AR_cavium=$(caviumprefix)ar
CC=$(caviumprefix)gcc
STRIP=$(caviumprefix)strip
#THE LAST PART OF THE NEXT LINE WAS REMOVED AND NOW YOU CAN COMPILE WITH THE CROSS COMPILE AND RUN IN THE ARMS
LDFLAGS=-Wl,--rpath,/slib -Wl,-dynamic-linker,/slib/ld-uClibc.so.0
CFLAGS=-march=armv4 -Os -lutil

all: xuartctl

xuartctl: xuartctl.c xuartcore.c
	${CC} ${CFLAGS} ${LDFLAGS} xuartctl.c -o xuartctl
	${STRIP} xuartctl


simple: xuartctl.c xuartcore.c
	gcc -lutil xuartctl.c -o xuartctl

rebuild: clean xuartctl

clean:
	rm -rf *.o xuartctl
