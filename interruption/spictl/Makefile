LDFLAGS=-fprofile-arcs
CFLAGS=-march=armv4 -Os -Wall -fprofile-arcs -ftest-coverage
CC=/home/workspace/mia_vita/ts7500_kernel/arm-uclibc-3.4.6/bin/arm-linux-gcc

all: spictl

spictl: spictl.c cavium.c peekpoke.c
	${CC} ${CFLAGS} ${LFLAGS} spictl.c cavium.c peekpoke.c -o spictl

rebuild: clean spictl

clean:
	rm -rf *.o spictl
