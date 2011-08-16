LDFLAGS=-fprofile-arcs
CFLAGS=-march=armv4 -Os -Wall -fprofile-arcs -ftest-coverage
CC=/home/workspace/mia_vita/ts7500_kernel/arm-uclibc-3.4.6/bin/arm-linux-gcc

all: spictl

spictl: spictl.c cavium.c
	${CC} ${CFLAGS} ${LFLAGS} spictl.c cavium.c -o spictl

rebuild: clean spictl

caller: caller.c
	${CC} ${CFLAGS} ${LFLAGS} caller.c -o caller

clean:
	rm -rf *.o spictl
