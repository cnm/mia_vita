LDFLAGS=-fprofile-arcs
CFLAGS=-march=armv4 -Os -Wall -fprofile-arcs -ftest-coverage
CC=/home/workspace/mia_vita/ts7500_kernel/arm-uclibc-3.4.6/bin/arm-linux-gcc

spictl.o: spictl.c cavium.c peekpoke.c
	${CC} ${CFLAGS} spictl.c cavium.c peekpoke.c -o spictl.o

spictl: spictl.o
	${CC} ${LFLAGS} spictl.o -o spictl

clean:
	rm -rf *.o spictl
