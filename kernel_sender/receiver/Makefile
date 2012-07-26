CC=gcc
#CC=../../crosstool/arm-unknown-linux-gnu/bin/arm-unknown-linux-gnu-gcc
#CFLAGS=-g
#CFLAGS += -D__GPS__
#CFLAGS += -D__DEBUG__
EXE=receiver

all: syscall_wrapper.o list.o miavita_packet.h main.c
	$(CC) $(CFLAGS) -o $(EXE) main.c syscall_wrapper.o list.o

list.o: list.h list.c
	$(CC) $(CFLAGS) -c list.c

syscall_wrapper.o: syscall_wrapper.h syscall_wrapper.c
	$(CC) $(CFLAGS) -c syscall_wrapper.c

gps_time.o: gps_time.h gps_time.c
	$(CC) $(CFLAGS) -c gps_time.c

gps_uartctl.o: gps_uartctl.h gps_uartctl.c
	$(CC) $(CFLAGS) -c gps_uartctl.c

init_counter: syscall_wrapper.o gps_time.o gps_uartctl.o init_counter.c
	$(CC) $(CFLAGS) -o init_counter init_counter.c syscall_wrapper.o gps_time.o gps_uartctl.o -lpthread

clean:
	rm -f *~ *.o $(EXE) init_counter

very_clean: clean
	rm -f *.json *.bin
